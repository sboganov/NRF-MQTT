#include <cstdlib>
#include <iostream>
#include <RF24/RF24.h>
#include "RF24Network.h"
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <mosquitto.h>

/**
 * g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
 **/
using namespace std;


struct RadioPacket {
	char name[11];
	char data[11];
};


RadioPacket rPacket;

RF24 radio(2,10 );
RF24Network network(radio);

pthread_mutex_t mutex;

// Address of our node
const uint16_t this_node = 0;
#define mqtt_host "localhost"
#define mqtt_port 1883


void *processNetworkRequests(void *) {
	int sockfd, newsockfd, portno = 51717, clilen;
     	struct sockaddr_in serv_addr, cli_addr;

	printf( "using port #%d\n", portno );
    
     	sockfd = socket(AF_INET, SOCK_STREAM, 0);
     	if (sockfd < 0) 
         perror( const_cast<char *>("ERROR opening socket") );
     	bzero((char *) &serv_addr, sizeof(serv_addr));

     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons( portno );
     	if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)  {
       		perror( const_cast<char *>( "ERROR on binding" ) );
		exit(1);
	}
     	listen(sockfd,5);
     	clilen = sizeof(cli_addr);


	while( 1 ) {
		printf( "waiting for new client...\n" );
        	if ( ( newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen) ) > 0 ) {
       			write(1, "opened new communication with client\n",38 );

          		//---- wait for a number from client ---
			char buffer[256];
  			int n;
			
			if ( (n = read(newsockfd,buffer,256) ) <= 0 )
				break;
			buffer[n] = '\0';
			printf("Receive data: '%s'\n", buffer);
			printf("Send test ping packet........................\n");
			memset(&rPacket,0,sizeof(RadioPacket));

			int nodeId = 4;
			char name[11];
			char value[11];
			sscanf(buffer, "%d %s %s", &nodeId, name, value);
	
			pthread_mutex_lock(&mutex);
			RF24NetworkHeader sendHeader(nodeId);
	
			strcpy(rPacket.name,name);
			strcpy(rPacket.data,value);
			int ok = network.write(sendHeader, &rPacket, sizeof(RadioPacket));
			printf("Status = %d\n", ok );
			pthread_mutex_unlock(&mutex);

        		close( newsockfd );
		}
	}
	return NULL;
}


void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback\r\n");
}
	

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("CLL %s %s\r\n", message->topic, (char * )message->payload);
}

// Structure of our payload
int main(int argc, char** argv) {


     	printf("Start\n");

	radio.begin();
	network.begin(/*channel*/0x5a, /*node address*/this_node);
	radio.printDetails();
	printf("-----------------------------------------------------\n");
	mosquitto_lib_init();
	struct mosquitto *mosq;
	mosq = mosquitto_new("NRFBridge", true, NULL);
	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);
	mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);
	mosquitto_subscribe(mosq, NULL, "#", 0);

	while (1) {
		mosquitto_loop(mosq, -1, 1);
		network.update();
		uint16_t nodes[255];
		while (network.available()) {
			// If so, grab it and print it out
			memset(&rPacket,0,sizeof(RadioPacket));
			RF24NetworkHeader header;
			size_t len = network.read(header, &rPacket, sizeof(RadioPacket));
			printf("Received packet # %s length=%d ", header.toString(),len);
			printf( "name = %s data = '%s'\n", rPacket.name, rPacket.data);

			if( nodes[header.from_node] != header.id) {
				nodes[header.from_node] = header.id;
				static char buf[2048];
				snprintf(buf,2048,"/stateUpdates/Node_%02d_%s/state", header.from_node, rPacket.name);
				mosquitto_publish(mosq, NULL, buf, strlen(rPacket.data),rPacket.data, 0, false);

			} else {
				printf(".... ignored\n");
			}
	
		}
		delay(1);
	}
	return 0;
}


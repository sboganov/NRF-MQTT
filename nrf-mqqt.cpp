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

/**
 * g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
 **/
using namespace std;


struct RadioPacket {
	char name[11];
	char data[11];
};


RadioPacket rPacket;

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and GPIO 25 CSN with SPI Speed @ 1Mhz
// RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_18, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ); 

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
// RF24 radio(RPI_V2_GPIO_P1_12, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
RF24 radio(2,10 );
RF24Network network(radio);

pthread_mutex_t mutex;

// Address of our node
const uint16_t this_node = 0;


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

// Structure of our payload
int main(int argc, char** argv) {


     	printf("Start\n");

	radio.begin();
	printf("Begin\n");
	network.begin(/*channel*/0x5a, /*node address*/this_node);
	radio.printDetails();
	printf("-----------------------------------------------------\n");

	pthread_t pid;
//	pthread_create(&pid, NULL, &processNetworkRequests, NULL);

//	pthread_mutex_init(&mutex, NULL);

	while (1) {
//		pthread_mutex_lock(&mutex);
		network.update();
//printf("UPDATE\r\n");
		uint16_t nodes[255];
		while (network.available()) {

printf("AVAIL\r\n");
			// If so, grab it and print it out
			memset(&rPacket,0,sizeof(RadioPacket));
			RF24NetworkHeader header;
			size_t len = network.read(header, &rPacket, sizeof(RadioPacket));
			printf("Received packet # %s length=%d ", header.toString(),len);
			printf( "name = %s data = '%s'\n", rPacket.name, rPacket.data);
			static char buf[2048];
			if( nodes[header.from_node] != header.id) {
				nodes[header.from_node] = header.id;
				sprintf(buf,"curl --silent -X PUT -H \"Content-Type: text/plain\" http://localhost/rest/items/Node_%o_%s/state -d \"%s\" &",header.from_node, rPacket.name, rPacket.data);
				system(buf);
				printf("CMD >> %s\n", buf);
			} else {
				printf(".... ignored\n");
			}
	
		}
//		pthread_mutex_unlock(&mutex);
		delay(10);
	}
	return 0;
}


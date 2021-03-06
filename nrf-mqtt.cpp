#include <cstdlib>
#include <iostream>
#include "RCSwitch.h"
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


void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback\r\n");
}
	

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	static char buf[1024];
	strncpy(buf,&(message->topic[9]), 1024);
	char *f = strchr(buf,'_');
	if(f != NULL) {
		int nodeId;
		sscanf(&f[1],"%d",&nodeId);
		char *name = strchr(&f[1],'_') + 1 ;
		RF24NetworkHeader sendHeader(nodeId);
	
		strcpy(rPacket.name,name);
		strcpy(rPacket.data,(char *)message->payload);
		int done = false;
		int count = 0;
		do {
			done = network.write(sendHeader, &rPacket, sizeof(RadioPacket));
			if(!done) {
				delay(300);

				count++;
				printf("Resend: %d\n",count);
			}
		} while(!done && count < 10);
		printf("SENDCMD Node: %d %s=%s Status = %d\n", nodeId,name,rPacket.data, done );
	}

}

// Structure of our payload
int main(int argc, char** argv) {


     	printf("Start\n");
	wiringPiSetup();

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
	mosquitto_subscribe(mosq, NULL, "/command/#", 0);

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
	}
	return 0;
}


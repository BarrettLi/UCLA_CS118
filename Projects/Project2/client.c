/* CS480, Spring 2016
*
* Simple UDP client, to demonstrate use of the sockets API
* Compile with:
* gcc -Wall -o udp-client udp-client.c
* or
* gcc -g -Wall -o udp-client udp-client.c # to support debugging
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

/* CONSTANTS */
#define REPLYPORT 12345	// I will bind to this port to call recvfrom()
// #define SERVERPORT 54321
#define MAX_SEQNUM (30720 - 1)
#define MAXPACKETSIZE 1024
#define PACKET_DATASIZE 1000
#define PACKET_HEADERSIZE (MAXPACKETSIZE - PACKET_DATASIZE)
#define DEBUG 0
#define SPEC_STDOUT 1
#define MAX_FILESIZE 100*1024*1024
#define SAVEFILENAME "received.data"
#define ENABLE_TIMEOUT 1
#define RTO	500000000L // retransmission timout out in nanoseconds

/*Global variables*/
int sockfd_sendto;
int sockfd_recvfrom;
unsigned int fileIndex = 0;
struct sockaddr_in my_addr, server_addr, recvfrom_addr;

uint16_t curr_SEQnum = 0;
uint16_t their_SEQnum = 0;
uint16_t expected_SEQnum = 0;
int threeWayCompleted = 0;
int receivedFIN = 0;
int sentFIN = 0;
int beginShutdown = 0;

struct UDP_PACKET {
	char SYN;
	char FIN;
	char ACK;
	char flag; // flag is 0 for the last packet (of the file)
	uint16_t ACKnum;
	uint16_t SEQ;
	int reserved[4];
	char DATA[1000];
};

char packetBuffer[MAXPACKETSIZE];
char fileBuffer[MAX_FILESIZE];
char* filenameArgument;
size_t filenameArgument_sizeof;

// The following 9variables deal with timeout retransmission
struct UDP_PACKET threeWayInitiatePacket;
struct UDP_PACKET threeWayResponsePacket;
struct UDP_PACKET FINPacket;
int threeWayInitiatePacketAcked = 0;
int threeWayResponsePacketAcked = 0;
int FINPacketAcked = 0;
pthread_t threeWayInitiatePacket_thread;
pthread_t FIN_packet_thread;
pthread_t threeWayResponsePacket_thread;

/* some prototypes */
// void establishConnection(int argc, char** argv); // not used anymore
void handle_error(const char* s);
int setup_bind(unsigned short port, struct sockaddr_in* my_addr);
uint16_t safe_add(uint16_t num, uint16_t addhowmany);
uint16_t safe_subtract(uint16_t num, uint16_t subtracthowmany);
void handle_packet(ssize_t num_received, struct UDP_PACKET packet);
void cleanup();
// The following 3 functions deal with timeout retransmission
void retransmitThreeWayInitatePacketUponTimeout();
void retransmitThreeWayResponsePacketUponTimeout();
void retransmitFINPacketUponTimeout();

// #include "establishConnection.h" // Don't use anymore
#include "setup_bind.h"
#include "safe_add.h"
#include "handle_packet.h"
#include "timeout_retransmission.h"






int main(int argc, char** argv)
{
	sockfd_recvfrom = setup_bind(REPLYPORT, &my_addr);
	struct hostent* hostentp;
	char* endptr;
	unsigned int portl;
	unsigned short port;
	size_t num_to_send;
	ssize_t num_sent;
	if (argc != 4) {
		fprintf(stderr, "usage: %s <hostname> <port> <filename>\n", argv[0]);
		exit(1);
	}
	filenameArgument = strdup(argv[3]);
	if (filenameArgument == NULL) {
		handle_error("strdup");
	}
	/* convert from string to int */
	portl = strtol(argv[2], &endptr, 10);
	if (endptr == NULL || portl == 0)
		handle_error("strtol");
	assert(portl < USHRT_MAX);
	port = (unsigned short)portl;
	if (!(hostentp = gethostbyname(argv[1]))) {
		herror("gethostbyname");
		exit(1);
	}
	if ((sockfd_sendto = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		handle_error("socket");
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr,
	       hostentp -> h_addr_list[0],
	       sizeof(struct in_addr));
	server_addr.sin_port = htons(port);
	/*	printf("I am about to send %s to IP address %s and port %d\n",
		       argv[3], inet_ntoa(server_addr.sin_addr), atoi(argv[2]));*/
	// num_to_send = strlen(argv[3]);
	num_to_send = sizeof(struct UDP_PACKET);
	// build the first SYN packet;
	struct UDP_PACKET SYN_packet;
	srand(time(NULL));
	curr_SEQnum = (uint16_t)rand() % (MAX_SEQNUM + 1);
	if (DEBUG)
		printf("initial sequence number is %d\n", curr_SEQnum);
	SYN_packet.SYN = 1;
	SYN_packet.FIN = 0;
	SYN_packet.ACK = 0;
	SYN_packet.flag = 1;
	SYN_packet.SEQ = curr_SEQnum;
	threeWayInitiatePacket = SYN_packet;
	// send the first FIN packet
	num_sent = sendto(sockfd_sendto, /* socket */
	                  // argv[3], /* buffer to send */
	                  (void*)&SYN_packet, /* buffer to send */
	                  num_to_send, /* number of bytes to send */
	                  0, /* flags=0: bare-bones use case*/
	                  (const struct sockaddr*)&server_addr, /* the destination */
	                  sizeof(server_addr)); /* size of the destination struct */
	if (DEBUG) {
		printf("I just sent %ld bytes!\n", num_sent);
		printf("Just sent packet:\n");
		printf("SYN:%d, ", SYN_packet.SYN);
		printf("FIN:%d, ", SYN_packet.FIN);
		printf("ACK:%d, ", SYN_packet.ACK);
		printf("flag:%d, ", SYN_packet.flag);
		printf("ACKnum:%d, ", SYN_packet.ACKnum);
		printf("SEQ:%d", SYN_packet.SEQ);
		printf("\n");
	}
	if (SPEC_STDOUT) {
		printf("Sending packet %d ", SYN_packet.SEQ);
		if (SYN_packet.SYN == 1) {
			printf("SYN ");
		}
		if (SYN_packet.FIN == 1) {
			printf("FIN ");
		}
		printf("\n");
	}


	curr_SEQnum = safe_add(curr_SEQnum, 1);
	if (num_sent < 0) {
		handle_error("sendto");
	}
	pthread_create(&threeWayInitiatePacket_thread, NULL, (void *) &retransmitThreeWayInitatePacketUponTimeout, NULL);
	// start to receive packets
	ssize_t num_received;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	while ((num_received = recvfrom(sockfd_recvfrom, /* socket */
	                                (void*)packetBuffer, /* buffer */
	                                MAXPACKETSIZE, /* size of buffer */

	                                0, /* flags = 0 */
	                                /* who’s sending */
	                                (struct sockaddr*)&recvfrom_addr,
	                                /* length of buffer to receive peer info */
	                                &addrlen)) > 0) {
		assert(num_received >= PACKET_HEADERSIZE);
		struct UDP_PACKET receivedPacket;
		memcpy((void*)&receivedPacket, (const void*)packetBuffer, (size_t)num_received);
		if (DEBUG) {
			printf("I just received %ld bytes!\n", num_received);
			printf("Just received packet:\n");
			printf("SYN:%d, ", receivedPacket.SYN);
			printf("FIN:%d, ", receivedPacket.FIN);
			printf("ACK:%d, ", receivedPacket.ACK);
			printf("flag:%d, ", receivedPacket.flag);
			printf("ACKnum:%d, ", receivedPacket.ACKnum);
			printf("SEQ:%d", receivedPacket.SEQ);
			printf("\n");
		}
		if (SPEC_STDOUT) {
			printf("Receving packet %d\n", receivedPacket.ACKnum);
		}
		handle_packet(num_received, receivedPacket);
	}
	exit(0);
}

void handle_error(const char* s) {
	perror(s);
	exit(1);
}
void cleanup() {
	pthread_join(threeWayInitiatePacket_thread, NULL);
	pthread_join(FIN_packet_thread, NULL);
	pthread_join(threeWayResponsePacket_thread, NULL);
}
#ifndef HELPERS
#define HELPERS
#define MAX_CLIENTS 100 

#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>
#include "queue.h"
#define ACTIVE 1
#define INACTIVE 0
#define SF 1
#define NOSF 0

// Macro preluat din cursul de SO
#include <errno.h>
#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(errno);					\
		}							\
	} while (0)

struct topic {
	char name[50];
	char sf;
};

// TCP client strcture
struct tcp_client {
	char id[11];
	int sockfd;
	int status; // ACTIVE OR INACTIVE 
	struct topic topics[100];
	int topic_arr_size;
	struct queue *queued_messages;
}tcp_client;

typedef struct udp_message {
	char topic[50];
	uint8_t data_type;
	char payload[1500];
} udp_message;

typedef struct tcp_message {
	struct udp_message udp_message;
	struct sockaddr_in udp_client;
} tcp_message;

typedef struct subscription {
	char subscribe[15]; // SUBSCRIBE / UNSUBSCRIBE
	char topic[50];
	int sf;
} subscription;


#endif
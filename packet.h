#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define PACKET_SIZE 100
#define PERCENT_LOSS 10
#define TIMEOUT 2
#define SOURCE "text.txt"
#define DEST "dest.txt"

typedef enum packettype { data = 1, ak = 2} type;

typedef struct
{
	char payload[PACKET_SIZE];
	int size;
	int seq;
	bool last;
	type packettype;
	int channel;
} packet;

typedef struct buff
{
	packet pkt;
	struct buff *next;
} packetbuffer;


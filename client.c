#include <stdio.h>
#include "packet.h"

//For displaying errors and exiting if needed

void printerror(char* error, int toclose)
{
	printf("Error while %s\n", error);
	if(toclose==1)
		exit(0);
}

//Trace

void trace(packet pkt)
{
	if(pkt.packettype == data)
	{
		printf("\nSENT PACKET: Sequence Number %d of Size %d Bytes from Channel %d \n", pkt.seq, pkt.size, pkt.channel);
	}
	else
	{
		printf("\nRCVD ACK for packet with Sequence Number %d of Size %d Bytes from Channel %d \n", pkt.seq, pkt.size, pkt.channel);
	}
}

int main(void)
{

	//Socket Descriptors

	int fd1;
	int fd2;

	//Declarations of essentials

	packet data1;		//to store packets
	packet data2;
	packet ack1;
	packet ack2;
	
	int lastackreceived;	//whether ack for last file has been received

	int tempsize;		//to store attributes
	int tempseq;
	int templast;		
	int onlyone;		//whether only the first channel has been used
	int channelclose1;	//to signify that channel 1 has done its job
	int channelclose2;	//to signify that channel 2 has done its job

	char buffer[PACKET_SIZE];	//buffer for message passing

	struct sockaddr_in server;	//server address

	fd_set rdfds;				//fdset for select
	struct timeval tv;			//timeval for timer

	//Initialization

	templast = 0;
	FD_ZERO(&rdfds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	lastackreceived = 0;
	onlyone = 1;
	tempsize = 0;
	channelclose1 = 0;
	channelclose2 = 0;

	//Sockets

	if((fd1 = socket(AF_INET, SOCK_STREAM, 0))<0)
		printerror("creating first socket", 1);
	if((fd2 = socket(AF_INET, SOCK_STREAM, 0))<0)
		printerror("creating second socket", 1);

	//Zeroing out memories
	memset(&server, 0, sizeof(server));
	memset(buffer, 0, sizeof(buffer));

	//Server Address

	server.sin_family = AF_INET;
	server.sin_port = htons(12345);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	//Connecting to server

	if((connect(fd1, (struct sockaddr*) &server, sizeof(server)))<0)
		printerror("connecting first socket", 1);
	if((connect(fd2, (struct sockaddr*) &server, sizeof(server)))<0)
		printerror("connecting second socket", 1);
	
	FD_SET(fd1, &rdfds);
	FD_SET(fd1, &rdfds);

	//Opening file to be read

	FILE *fp;
	fp = fopen(SOURCE, "rb");

	//Create packet 1
	
	tempsize = fread(buffer, 1, PACKET_SIZE, fp);

	tempseq = 1;
	if(feof(fp))
	{
		templast = 1;
	}

	sprintf(data1.payload, "%s", buffer);
	// data1.payload[tempsize]='\0';
	data1.size = tempsize;
	data1.seq = tempseq;
	data1.last = templast;
	data1.packettype = data;
	data1.channel = 1;

	//Send packet 1
	
	while((write(fd1, (const void *) &data1, sizeof(data1)))<0)
	{
		printerror("sending packet from first channel\n", 0);
	}
	trace(data1);

	tempseq++;
	
	//Create packet 2
	
	if(templast==0)
	{
		onlyone = 0;

		tempsize = fread(buffer, 1, PACKET_SIZE, fp);
		tempseq = 2;
		if(feof(fp))
		{
			templast = 1;
		}
		
		sprintf(data2.payload, "%s" , buffer);
		data2.payload[tempsize]='\0';
		data2.size = tempsize;
		data2.seq = tempseq;
		data2.last = templast;
		data2.packettype = data;
		data2.channel=2;

		//Send packet 2
		
		while((write(fd2, (const void *) &data2, sizeof(data2)))<0)
		{
			printerror("sending packet from second channel\n", 0);
		}
		trace(data2);

		tempseq++;
	}
	

	int bytesRecvd;

	while(lastackreceived==0||(channelclose1==0||channelclose2==0))
	{
		//Add fds to monitor and reset timer

		FD_ZERO(&rdfds);
		FD_SET(fd1, &rdfds);
		FD_SET(fd2, &rdfds);
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;

		//Now, both the channels are waiting for acks

		int retval = select(10, &rdfds, NULL, NULL, &tv);

		if(retval==-1)
		{
			printerror("select\n", 1);
		}
		else if(retval)
		{
			//One of the channels has received an ack

			if(FD_ISSET(fd1, &rdfds))
			{
				//First channel has received the ack
				while((bytesRecvd = recv(fd1, &ack1, sizeof(ack1), 0))==0);
				if(bytesRecvd<0)
				{
					printerror("receiving at channel 1\n", 0);
				}
				else
				{
					if(ack1.seq==data1.seq)
					{
						//ACK successfully received
						
						trace(ack1);

						//Check for last
						if(ack1.last==1)
						{
							lastackreceived=1;
							channelclose1 = 1;
						}

						if(channelclose1!=1)
						{
							//First channel is still functional

							//Create packet 1

							tempsize = fread(buffer, 1, PACKET_SIZE, fp);

							if(tempsize==0)
								channelclose1 = 1;		//file end reached
							else
							{
								if(feof(fp))
								{
									templast = 1;
								}
			
								sprintf(data1.payload, "%s", buffer);
								data1.payload[tempsize]='\0';
								data1.size = tempsize;
								data1.seq = tempseq;
								data1.last = templast;
								data1.packettype = data;
								data1.channel = 1;
				
								//Send packet 1
					
								while((write(fd1, (const void *) &data1, sizeof(data1)))<0)
								{
									printerror("sending packet from first channel\n", 0);
								}
								trace(data1);
								
								tempseq++;
							}
						}
					}
				}
			}
			// else
			// {
			// 	//timeout for first channel
			// 	while((write(fd1, (const void *) &data1, sizeof(data1)))<0)
			// 	{
			// 		printerror("sending packet from first channel\n", 0);
			// 	}
			// 	trace(data1);
			// }
			
			if(FD_ISSET(fd2, &rdfds))
			{
				//First channel has received the ack
				while((bytesRecvd = recv(fd2, &ack2, sizeof(ack2), 0))==0);
				if(bytesRecvd<0)
				{
					printerror("receiving at channel 2\n", 0);
				}
				else
				{
					if(ack2.seq==data2.seq)
					{
						//ACK successfully received
						
						trace(ack2);

						//Check for last
						if(ack2.last==1)
						{
							lastackreceived=1;
							channelclose2 = 1;
						}

						if(channelclose2!=1)
						{
							//if second channel is still functional

							//Create packet 2
			
							tempsize = fread(buffer, 1, PACKET_SIZE, fp);

							if(tempsize==0)
								channelclose2 = 1;	//File end reached
							else
							{

								if(feof(fp))
								{
									templast = 1;
								}
			
								sprintf(data2.payload, "%s" , buffer);
								data2.payload[tempsize]='\0';
								data2.size = tempsize;
								data2.seq = tempseq;
								data2.last = templast;
								data2.packettype = data;
								data2.channel = 2;
				
								//Send packet 2
					
								while((write(fd2, (const void *) &data2, sizeof(data2)))<0)
								{
									printerror("sending packet from second channel\n", 0);
								}
								trace(data2);
								
								tempseq++;
							}
						}
					}
				}
			}
			// else
			// {
			// 	//timeout for second channel, if it has been used
			// 	if(onlyone==0)
			// 	{
			// 		while((write(fd2, (const void *) &data2, sizeof(data2)))<0)
			// 		{
			// 			printerror("sending packet from first channel\n", 0);
			// 		}
			// 		trace(data2);
			// 	}
			// }
			
		}
		else
		{
			//timeout for both the channels

			printf("Timeout!!\n");

			if(channelclose1!=1)
			{
				while((write(fd1, (const void *) &data1, sizeof(data1)))<0)
				{
					printerror("sending packet from first channel\n", 0);
				}
				trace(data1);
			}

			if(onlyone==0&&channelclose2!=1)
			{
				while((write(fd2, (const void *) &data2, sizeof(data2)))<0)
				{
					printerror("sending packet from first channel\n", 0);
				}
				trace(data2);
			}
		}
	}
	
	close(fd1);
	close(fd2);
}

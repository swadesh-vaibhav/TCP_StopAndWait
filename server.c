#include <stdio.h>
#include "packet.h"

//For displaying errors and exiting if needed

int seeking;

packetbuffer * writetofile(packetbuffer *head, FILE* fp, packet current)
{
    // printf("seek -> %d,  current -> %d\n", seeking, current.seq);
    if(current.seq==seeking)
    {
        // printf("Insert current\n");
        fwrite(current.payload, 1, current.size, fp);
        seeking++;
        while(head!=NULL && head->pkt.seq==seeking)
        {
            // printf("Insert old");
            fwrite(head->pkt.payload, 1, head->pkt.size, fp);
            seeking++;
            head = head->next;
        }
    }
    else if(head==NULL)
    {
        // printf("Insert into bufferhead\n");
        head = (packetbuffer *)malloc(sizeof(packetbuffer));
        head->next =NULL;
        head->pkt = current;
    }
    else
    {
        // printf("Insert into buffer\n");
        packetbuffer *trav=head;
        while(trav->next!=NULL)
            trav=trav->next;
        
        packetbuffer *temp = (packetbuffer *)malloc(sizeof(packetbuffer));
        temp->next =NULL;
        temp->pkt = current;
        trav->next = temp;
    }
    return head;
}

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
		printf("\nRCVD PACKET: Sequence Number %d of Size %d Bytes from Channel %d \n", pkt.seq, pkt.size, pkt.channel);
	}
	else
	{
		printf("\nSENT ACK for packet with Sequence Number %d of Size %d Bytes from Channel %d \n", pkt.seq, pkt.size, pkt.channel);
	}
}

int main(void)
{

	//Socket Descriptors

	int fd;
    int fd1;
    int fd2;

	//Declarations of essentials

	packet data1;
	packet data2;
	packet ack1;
	packet ack2;

	int tempsize;
	int tempseq;
	int templast;

	char buffer[PACKET_SIZE];
    packetbuffer *bhead;

	struct sockaddr_in server;

	fd_set rdfds;
	struct timeval tv;

    int opt;
    int lastreceived;

	//Initialization

	templast = 0;
	FD_ZERO(&rdfds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
    opt = 1;
    bhead = NULL;
    lastreceived = 0;
    seeking = 1;

    memset(buffer, 0, sizeof(buffer));
    memset(&server, 0, sizeof(server));

    srand(time(0));

	//Sockets

	if((fd = socket(AF_INET, SOCK_STREAM, 0))<0)
		printerror("creating socket", 1);

	//Server Address

	server.sin_family = AF_INET;
	server.sin_port = htons(12345);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
    //Binding

    if((bind(fd, (struct sockaddr*) &server, sizeof(server)))<0)
        printerror("binding\n", 1);
    
    //Allow multiple connections

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        printerror("setting setsockopt", 1);   
    } 

	//Listening for two connections

	if (listen(fd, 2) < 0)   
    {   
        printerror("activating listen", 1);     
    }   
     
    //Accept both the connections

    if ((fd1 = accept(fd, (struct sockaddr *) NULL, NULL))<0)   
    {
        printerror("accepting first connection", 1);  
    } 
    if ((fd2 = accept(fd, (struct sockaddr *) NULL, NULL))<0)   
    {   
        printerror("accepting second connection", 1);
    } 

    //Add both of these to read fdset
	
	FD_SET(fd1, &rdfds);
    FD_SET(fd2, &rdfds);

	//Opening file to be read

	FILE *fp;
	fp = fopen(DEST, "wb");

	

	while(lastreceived==0||bhead!=NULL)
	{
		//Now, both the channels are waiting for data
        
	    FD_ZERO(&rdfds);
        FD_SET(fd1, &rdfds);
        FD_SET(fd2, &rdfds);

		int retval = select(10, &rdfds, NULL, NULL, &tv);
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;

		if(retval==-1)
		{
			printerror("selecting \n", 1);
		}
		else if(retval)
		{
			//One of the channels has received an data

			if(FD_ISSET(fd1, &rdfds))
			{
				//First channel has received the data

				int bytesRecvd = read(fd1, &data1, sizeof(data1));
                
                //Implement Packet loss

                int randval = rand()%100;
                if(randval>=PERCENT_LOSS)
                {
                    //Not dropped
                    
                    trace(data1);

                    if(bytesRecvd<0)
                    {
                        printerror("receiving at channel 1\n", 0);
                    }
                    else
                    {
                        if(data1.seq>=seeking)
                        {

                            //Data successfully received
                            
                            bhead = writetofile(bhead, fp, data1);
                            if(data1.last==1)
                                lastreceived=1;
                        }
                        //Create packet 1
        
                        ack1 = data1;

                        ack1.packettype = ak;
        
                        //Send packet 1
            
                        while((send(fd1, (const void *) &ack1, sizeof(ack1), 0))<0)
                        {
                            printerror("sending packet from first channel\n", 0);
                        }
                        trace(ack1);
                    }
				}
                else
                {
                    // printf("Dropped ");
                    // trace(data1);
                }

				FD_SET(fd1, &rdfds);
			}

			if(FD_ISSET(fd2, &rdfds))
			{

				//Second channel has received the data
				int bytesRecvd = read(fd2, &data2, sizeof(data2));

                //Implement Packet Loss
                int randval = rand()%100;
                if(randval>=PERCENT_LOSS)
                {
                    trace(data2);

                    if(bytesRecvd<0)
                    {
                        printerror("receiving at channel 2\n", 0);
                    }
                    else
                    {
                        if(data2.seq>=seeking)
                        {

                            //Data successfully received
                            
                            bhead = writetofile(bhead, fp, data2);
                            if(data2.last==1)
                                lastreceived=1;
                        }
                            //Create packet 2
            
                        ack2 = data2;
                        ack2.packettype = ak;
        
                        //Send packet 2
            
                        while((send(fd2, (const void *) &ack2, sizeof(ack2), 0))<0)
                        {
                            printerror("sending packet from second channel\n", 0);
                        }
                        trace(ack2);
                    }
                }
                else
                {
                    // printf("Dropped ");
                    // trace(data2);
                }
                

				FD_SET(fd2, &rdfds);
			}
		}
	}
	
	close(fd1);
	close(fd2);
}
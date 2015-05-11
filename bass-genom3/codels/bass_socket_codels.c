/* Copyright (c) 2014, LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "acbass.h"

#include "bass_c_types.h"

#include "Sockets.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>

#define MessageBufferSize	512
#define POLL_SIZE           32
#define infoSize            4   //int32_t x2 and int64_t x1 => int32_t xinfoSize
int sockfd, newsockfd, portno, clilen, n, yes=1, good=0, bad=0;
uint32_t total=0;
struct sockaddr_in serv_addr, cli_addr;
char buffer[MessageBufferSize];
int32_t *message, *aux, *messageInfo;
int64_t sizeofMessage;
struct pollfd poll_set[POLL_SIZE];
int numfds = 0;
int max_fd = 0;
int fd_index, i, end=0;
uint32_t num = 1;


/* --- Task socket ------------------------------------------------------ */


/** Codel sInitModule of task socket.
 *
 * Triggered by bass_start.
 * Yields to bass_ether.
 */
genom_event
sInitModule(genom_context self)
{
    end = 1;
    return bass_ether;
}


/* --- Activity DedicatedSocket ----------------------------------------- */

/** Codel initModule of activity DedicatedSocket.
 *
 * Triggered by bass_start.
 * Yields to bass_ether, bass_recv.
 */
genom_event
initModule(const bass_Audio *Audio, genom_context self)
{
    uint32_t i;
	//printf("\nSending information.\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0); /*TODO:rename sockfd to server_sockfd*/
    
	if(sockfd < 0)
	{
		printf("ERROR: Socket not opened.\n");
		return bass_ether;	
	}	
	printf("Socket opened.\n");
	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = 8081;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);


	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) 
	{
		printf("ERROR: setsockopt.\n");
		return bass_ether;
	} 

	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Error binding.\n");
		close(sockfd);
		return bass_ether;
	}
	printf("Bind correctly\n");
    
    listen(sockfd, 5); /*5 connections allowed on the incoming queue*/

    fd_set writefds, read_fds;
    struct timeval timeout;
    timeout.tv_usec = 5000; 
    FD_ZERO(&writefds);
    FD_SET(newsockfd, &writefds);
    
    numfds = 0;
    memset(poll_set, '\0', sizeof(poll_set));
    poll_set[numfds].fd = sockfd;
    poll_set[numfds].events = POLLIN;
    numfds++;
    max_fd = sockfd;

    end = 0;

    sizeofMessage = 2*(Audio->data(self)->nChunksOnPort*Audio->data(self)->nFramesPerChunk)*sizeof(int32_t);
    message = malloc(sizeofMessage);
    aux = malloc(sizeofMessage);

    messageInfo = malloc(infoSize*sizeof(int32_t));

    return bass_recv;
}

/** Codel Transfer of activity DedicatedSocket.
 *
 * Triggered by bass_recv.
 * Yields to bass_recv, bass_ether.
 */
genom_event
Transfer(const bass_Audio *Audio, genom_context self)
{
    char *result;
    int32_t position, len, j, blocksToSend;
    int32_t pow, clientIndex, serverIndex, auxpow;
    int32_t CAPTURE_PERIOD_SIZE, CAPTURE_CHANNELS;
    int64_t nfr;
    int N, loss, nFrames;
    int32_t *l, *li, *r, *ri;
    binaudio_portStruct *data;

    if(end==0)
    {
        good = 0;
        bad = 0;
//        printf("[Before poll] numfds: %d\n", numfds);
//        printf("Current connections: ");
//        for(i=0; i<numfds; i++)
//            printf("%d ", poll_set[i].fd);  
//        printf("\n\n"); 
        poll(poll_set, numfds, 10);
//        printf("[After poll] numfds: %d\n", numfds);
        for(fd_index=0; fd_index<numfds; fd_index++)
        {
//            printf("{{Begining of for}} fd_index: %d - numfds: %d\n", fd_index, numfds);
            if(poll_set[fd_index].revents & POLLIN)
            {
                printf("[%d] received event POLLIN - fd_index %d \n",  poll_set[fd_index].fd, fd_index);
                if(poll_set[fd_index].fd == sockfd)
                {
                    clilen = sizeof(cli_addr);  /*TODO: rename cli_addr to client_sockfd*/
                    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); /*TODO: rename newsockfd to client_sockfd*/
                    poll_set[numfds].fd = newsockfd;
                    poll_set[numfds].events = POLLIN;
                    printf("New client [%d] added\n", poll_set[numfds].fd);
                    numfds++;
                    printf("[After adding new client] fd_index: %d - numfds: %d\n", fd_index, numfds);
                    printf("Current connections: ");
                    for(i=0; i<numfds; i++)
                        printf("%d ", poll_set[i].fd);   
                    printf("\n");   
                    break;              
                }
                else
                {
                    n = read(poll_set[fd_index].fd, buffer, MessageBufferSize-1);
                    if(n == 0)
                    {
                        close(poll_set[fd_index].fd);
                        poll_set[fd_index].events = 0;
                        printf("Removing client [%d]\n", poll_set[fd_index].fd);
                        poll_set[fd_index] = poll_set[numfds-1];      
                        numfds--;
                        //fd_index--;
                        printf("[After Removing] fd_index: %d - numfds: %d\n", fd_index, numfds);
                        printf("Current connections: ");
                        for(i=0; i<numfds; i++)
                            printf("%d ", poll_set[i].fd);  
                        printf("\n");              
                    }
                    else
                    {
                        printf("\nMessage received from [%d]: %s",poll_set[fd_index].fd, buffer);

                        if(strstr(buffer, "Read Port"))
                        {
                            N = (int) findValue(buffer, "N");
                            printf("[DEBUG]: N is %d\n", N);
                            nfr = findValue(buffer, "nfr");
                            printf("[DEBUG]: nfr is %d\n", nfr);
                            if(N>0 && nfr>0)
                            {
                                /* Read data from the port */
                                data = Audio->data(self);

                                nFrames = getAudioData(data, message, N, &nfr, &loss);

                                /*Send information related to Data*/
                                messageInfo[0] = nFrames;
                                messageInfo[1] = loss;
                                messageInfo[2] = (int32_t) nfr; // get the low 32 bits
                                messageInfo[3] = (nfr >> 32);   // get the high 32 bits

                                n = send(poll_set[fd_index].fd, messageInfo, infoSize*sizeof(int32_t), NULL);

                                if(n>0)
                                {
                                    good++;
                                    printf("SENT: n = %d bytes (%d samples)\n", n, n/4);
                                }
                                total = n/4;
                                while(total<infoSize)
                                {
                                    for(i=0; i<(infoSize-total); i++)
                                    {
                                        aux[i] = messageInfo[total+i];
                                    }
                                    n = send(poll_set[fd_index].fd, aux, i*sizeof(int32_t), NULL);
                                    if(n>0)
                                    {
                                        good++;
                                        total = total + n/4;
                                        printf("SENT: n = %d bytes (%d samples)\tTOTAL: n = %d bytes (%d samples)\n", n, n/4, total*4, total);
                                    }
                                    else
                                        bad++;
                                } 
                        
                                /*Send Data*/
                                printf("nFrames: %d\n", nFrames);
                
                                n = send(poll_set[fd_index].fd, message, 2*nFrames*sizeof(int32_t), NULL);

                                if(n>0)
                                {
                                    good++;
                                    printf("SENT: n = %d bytes (%d samples)\n", n, n/4);
                                }
                                total = n/4;
                                while(total<nFrames)
                                {
                                    for(i=0; i<(nFrames-total); i++)
                                    {
                                        aux[i] = message[total+i];
                                    }
                                    n = send(poll_set[fd_index].fd, aux, i*sizeof(int32_t), NULL);
                                    if(n>0)
                                    {
                                        good++;
                                        total = total + n/4;
                                        printf("SENT: n = %d bytes (%d samples)\tTOTAL: n = %d bytes (%d samples)\n", n, n/4, total*4, total);
                                    }
                                    else
                                        bad++;
                                }                                 
                            }
                        }                       
                    }
                }
            }
        }
        return bass_recv;
    }
    else
        return bass_ether;
}


/* --- Activity CloseSocket --------------------------------------------- */

/** Codel closeSocket of activity CloseSocket.
 *
 * Triggered by bass_start.
 * Yields to bass_ether.
 */
genom_event
closeSocket(genom_context self)
{
    end = 1;
    free(message);
    free(aux);
    free(messageInfo);
    for(i=0; i<numfds; i++)
        close(poll_set[i].fd);
    printf("Connections closed.\n"); 
        
    return bass_ether;
}

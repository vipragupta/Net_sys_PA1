//g++ -std=c++11 udp_client.cpp -o client -lcrypto -lssl

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <openssl/md5.h>

#define MAXBUFSIZE 1048576		//1048576 bytes or 10 MB
#define FILEPACKETSIZE 5*1024			

//This is the packet struct that will be used to transfer data between client and server.
struct packet
{
	int clientId;			//Id of client.
	char filename[100];		//File to send or receive,
	unsigned char data[FILEPACKETSIZE];	//File data.
	char command[50];			//The command that user entered
	int totalPackets;			//Total number of packets that will be sent.
	int seqNo;					//The current packet's seq num.
	int dataSize;				//Size of data.
	char mdHash[FILEPACKETSIZE];	//md5ash
	int ack;					// 0: data packet   1: ack
};

//Gives the size of the file in bytes.
size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
  	size_t file_size = ftell(file);
  	return file_size;
}

//Calculates the total number of packet transfer that will be required to transfer the whole file.
int getTotalNumberOfPackets(size_t file_size) {

	int fileSize = FILEPACKETSIZE;
	int packets = 0;
	packets = file_size/fileSize;
	
	//printf("Packets: %lu  %d\n", file_size, packets);
  	if (file_size % fileSize > 0) {
  		packets++;
  	}
  	
  	//printf("Packets: %d\n", packets);
  	return packets;
}

int getClientID() {
	srand (time(NULL));
  	int clientId = 0;
  	int i = 0;
  	while(i<4){
  		clientId = clientId * pow(10, i) + (char)(rand() % 10 + 1);
  		i++;
  	}
  	return clientId;
}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	int timeoutMicroseconds = 600000;
	char buffer[FILEPACKETSIZE];
	unsigned char file_buffer[FILEPACKETSIZE];
	int client = getClientID();
	struct sockaddr_in server;              //"Internet socket address structure"
	unsigned int server_length = sizeof(server);

	//Struct to make recvfrom timeout.
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;


	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	bzero(&server,sizeof(server));               //zero the struct
	server.sin_family = AF_INET;                 //address family
	server.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	server.sin_addr.s_addr = inet_addr(argv[1]); //sets server IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}
	//Setting the timeout for recvfrom as 600ms
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    	printf("Error Socket timeout");
	}

	//Keep looping giving options to user till he/she enters exit.
	while (1) {
		char option[100];
		char *options;

		bzero(option, sizeof(option));
		printf("\n*********************** MENU ***********************\n");
		printf(". put [FileName]\n. get [FileName]\n. del [FileName]\n. ls\n. exit\n\n");
		printf("Enter the operation you want to perform: ");
		
		fgets(option, sizeof(option), stdin);
		options = strtok(option, "\n");
		printf("--------------------------------------\n");
		printf("You selected:%s\n\n", option);

		char *optionCmd;
		char *filename;
		optionCmd = strtok(options, " ");
		
		//Move on only if user has entered something.
		if (optionCmd && optionCmd != NULL) {
			if (strcmp(optionCmd, "get") == 0 || strcmp(optionCmd, "put") == 0 || strcmp(optionCmd, "del") == 0 ) {
				filename = strtok(NULL, "");
				if (!filename) {
					printf("No File Name Entered. Please try again.\n");
					continue;
				}
			}
		} else {
			continue;
		}
		
		//Do the following if the user has entered put command and had given a file name too.
		if (strcmp(optionCmd, "put") == 0) {
			//Setting the timeout for recvfrom as 600ms
			tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;		

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
	    		printf("Error Socket timeout\n");
			}

			FILE *file;
			char filePath[50];
		    memset(filePath, '\0', sizeof(filePath));
			//strcat(filePath, "./clientDir/");
			strcat(filePath, filename)	;
			printf("filePath: %s\n", filePath);
			file = fopen(filePath, "rb");
			if(file == NULL)
		    {
		      printf("Given File Name does not exist\n");
		      continue;
		    }

		  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
		  	fseek(file, 0, SEEK_SET);

		  	int count = 0;
		  	int totalPackets = getTotalNumberOfPackets(file_size);		//Calculates the total number of packet transfer that will be required to transfer the whole file.
		  	printf("Total Packets: %d\n", totalPackets);

		  	//Iterate over till the packet sent has a sequence Number equal to total packets to be sent.
		  	while (count < totalPackets) {
		  		int retry = 0;
		  		printf("--------------------------------------\n");
		  		
		  		bzero(file_buffer,sizeof(file_buffer));
		  		bzero(buffer,sizeof(buffer));
			  	int byte_read = fread(file_buffer, sizeof(unsigned char), FILEPACKETSIZE, file);		  	
			  	if( byte_read <= 0)
			    {
			      printf("unable to copy file into file_buffer\n");
			      exit(1);
			    }
			    
			    struct packet pack;
			    pack.clientId = client;
			    memset(pack.filename, '\0', sizeof(pack.filename));
			    strcpy(pack.filename, filename);
				pack.dataSize = byte_read;
				pack.totalPackets = totalPackets;
				pack.seqNo = count;
				memset(pack.command, '\0', sizeof(pack.command));
			    strcpy(pack.command, "put");
			    memcpy(pack.data, file_buffer, sizeof(file_buffer));
			    
			    // This is how reliability is implemented.
			  	RESEND:
			  		printf("Packet: %d     bytes_read: %d\n", count, byte_read);
				    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));

				    if (nbytes < 0){
						printf("Error in sendto\n");
					}
					printf("Waiting for server ack..\n");
					struct packet receivedPacket;
					nbytes = recvfrom(sock, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&server, &server_length);  
					
					if (nbytes > 0 || receivedPacket.seqNo >= count && receivedPacket.ack == 1) {
						printf("Server Acknowledged Packet  %d\n", count);
					} else {
						//Re-transmit the packet if ack is not received.
						if (retry < 10) {
							printf("\nTimer timeout, Resending packet %d\n", count);
							retry++;
							goto RESEND;
						} else {
							printf("Resent %d Packet 5 times, Server Not responding. Please try after some time.\n ", count);
							break;
						}
					}
				count++;
			}
			if (count == 0) {
				printf("\nTotal Packets sent: %d   totalPackets: %d \n", count, totalPackets);
			} else {
				printf("\nTotal Packets sent: %d    totalPackets: %d \n", count - 1, totalPackets - 1 );
			}
			//The following code is what client does when get command is received.
		} else if (strcmp(optionCmd, "get") == 0) {
		    printf("\n");
		    system("clear");
		    printf("--------------------------------------\n");

		    //Build the initial packet to communicate server to send the file wit given filename.
		    struct packet pack;
		    pack.clientId = client;
		    memset(pack.filename, '\0', sizeof(pack.filename));
		    strcpy(pack.filename, filename);

			pack.seqNo = 0;
			memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "get");

		    printf("FileName Sent to server.\n");
		    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
		    printf("\n");
		    if (nbytes < 0){
				printf("Error in sendto\n");
			}

		  	bzero(file_buffer,sizeof(file_buffer));
		  	int packetExpected = 0;
		  	int waitTime = 0;

		  	//Setting the timeout for recvfrom as 600ms
		  	tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;
		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}

			printf("Waiting for first Packet...\n");
		  	
		  	//Loop till all packets of the file are sent.
		  	while (1) {
			  	struct packet client_pack;
			  	if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
			    {
			    	printf("Waiting for packet %d\n", packetExpected);
			    	waitTime++;
			    	//Wait fpr only 600 * 10 ms for each packet, else assume that server is down and client moves out from loop.
			    	if (waitTime > 10) {
			    		printf("Server not responding. Please try again after sometime.\n");
			    		break;
			    	}
			    	continue;
			    }
			    
	    		printf("Receive Packet number: %d\n", client_pack.seqNo);

			    if (client_pack.dataSize == -1) {
			    	printf("Server Says: Given File Name does not exist\n");
			    	break;
			    }
			    //Ack and store the packet only if its sequence num is same as the next packet expected.
			    if (packetExpected == client_pack.seqNo) {
			    	waitTime = 0;
				    printf("Received Packet: %s  %d\n", client_pack.filename, client_pack.seqNo);

				    FILE *file;
				    char filePath[50];
				    memset(filePath, '\0', sizeof(filePath));
	    			//strcat(filePath, "./clientDir/");
	    			strcat(filePath, filename)	;

				    if (packetExpected == 0) {
				    	file = fopen(filePath,"wb");
				    } else {
				    	file = fopen(filePath,"ab");
				   	}

				    unsigned char *file_buffer = client_pack.data;				
				    memcpy(file_buffer, client_pack.data, client_pack.dataSize);
				    //Write the received bytes to file.
				    int fileSize = fwrite(file_buffer , sizeof(unsigned char), client_pack.dataSize, file);
				    printf("Packet: %d     bytes_wrote: %d\n", packetExpected, fileSize);

				    if( fileSize < 0)
				    {
				    	printf("error writting file\n");
				        exit(1);
				    }

				    client_pack.ack = 1;
				    //Ack to server that packet has been received.
					nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
					fclose(file);
					if (client_pack.seqNo == client_pack.totalPackets - 1) {
						// nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
						// if (nbytes < 0){
						// 	printf("Error in sendto\n");
						// }
						break;
					}
					packetExpected++;
					//Ack again if server is sending a packet that client has already processed.
				} else if (packetExpected > client_pack.seqNo) {
					printf("Ack of %d sent again.\n", client_pack.seqNo);
					nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
				} else {
					waitTime++;
				}
				if (waitTime > 5) {
					printf("Server not sending the Packet Expected. Please try again.\n");
					if (remove(client_pack.filename));
					break;
				}
				printf("--------------------------------------\n");
			}
			//Delete command
		} else if (strcmp(optionCmd, "del") == 0) {
			//Setting the timeout for recvfrom as 600ms
			tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}

			struct packet pack;
		    pack.clientId = client;
		    
		    memset(pack.filename, '\0', sizeof(pack.filename));
		    strcpy(pack.filename, filename);

			pack.seqNo = 0;
			memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "del");
		    //Send the filename that server should delete.
		    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
		    if (nbytes < 0){
				printf("Error in sendto\n");
			}
			int countDel = 0;
			struct packet client_pack;
				while (countDel < 5) {
			  	
			  	if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
			    {
			    	printf("Waiting for server's response...\n");
			    	countDel++;
			    	continue;
			    } else {
			    	printf("Timed out\n");
			    	break;
			    }
			}
		    if (client_pack.dataSize == -1) {
				printf("Some error occured at server side.\n");
			} else {
				client_pack.ack = 1;
				//Send ack to server.
				nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
			    if (nbytes < 0){
					printf("Error in sendto\n");
				}
			    printf("Server Says: %s\n", client_pack.data);
	
			}
			//LS command
		} else if (strcmp(optionCmd, "ls") == 0 ) {

			//Setting the timeout for recvfrom as 600ms
			tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}
			struct packet pack;
		    pack.clientId = client;

			pack.seqNo = 0;
			memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "ls");
		    //Send ls command to server
		    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
		    if (nbytes < 0){
				printf("Error in sendto\n");
			}

		  	bzero(file_buffer,sizeof(file_buffer));
		  	struct packet client_pack;
		  	int countLs = 0;
		  	while (countLs < 5) {
			  	if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
			    {
			    	printf("Waiting for server's response...\n");
			    	countLs;
			    	continue;
			    } else {
			    	printf("Timed out\n");
			    	break;
				}
			}
		    if (client_pack.dataSize == -1) {
				printf("Some error occured at server side.\n");
			} else {
				//Send ack to server.
				client_pack.ack = 1;
				nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
			    if (nbytes < 0){
					printf("Error in sendto\n");
				}

				char *token;
				int tcount = 0;
				//Parse the received data and print.
    			token = strtok((char *)client_pack.data, "#");
    			while( token != NULL ) 
			    {
			    	printf("%s  \t", token);
			      	token = strtok(NULL, "#");
			      	tcount++;
			      	if (tcount % 5 == 0) {
			      		printf("\n");
			      	}
			    }
			    printf("\n\n");
			}
			//Exit gracefully
		} else if (strcmp(optionCmd, "exit") == 0) {
			struct packet pack;
		    memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "exit");
		    int exitCounter = 0;
		    RESEND_EXIT:
		    	//Tell server to exit the process.
		      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
				if (nbytes < 0){
					printf("Error in sendto\n");
					exitCounter++;
					if (exitCounter < 10) {
						goto RESEND_EXIT;
					} else {
						printf("Not able to send to Server\n");
					}
				}
			printf("Good Bye!\n");
			return 0;
		} else {
			struct packet pack;
		    memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, optionCmd);
	      	
	      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));

			if (nbytes < 0){
				printf("Error in sendto\n");
			}
			struct packet client_pack;
			if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
		    {
		    	printf("No Data Received\n");
		    }
		    //clear screen
		    if (strcmp(optionCmd, "clear") == 0) {
				system("clear");
			}
			printf("Server Says: %s \"%s\"\n", client_pack.data, client_pack.command);
		}
	}
	//Close the socket.
	close(sock);
}
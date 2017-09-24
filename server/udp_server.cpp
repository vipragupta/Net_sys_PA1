//g++ -std=c++11 udp_server.cpp -o server -lcrypto -lssl

#include <sys/types.h>
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
#include <string.h>
#include <dirent.h>
#include <openssl/md5.h>

#define MAXBUFSIZE 2000
#define FILEPACKETSIZE 5*1024		//1048576 bytes or 10 MB

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
  	if (file_size % fileSize > 0) {
  		packets++;
  	}
  	return packets;
}


FILE *getFilePointer(char filename[])
{
    FILE *file = NULL;
    char filePath[50];
   	memset(filePath, '\0', sizeof(filePath));
    strcat(filePath, "./serverDir/");
    strcat(filePath, filename)	;

    file = fopen(filePath, "r");
    if (file) {
        printf("File %s opened \n\n", filename);
        return file;
    }
    return NULL; // error
}

int main (int argc, char * argv[] )
{
	int sock;                              //This will be our socket
	int timeoutMicroseconds = 600000;

	struct sockaddr_in server, remote;     //"Internet socket address structure"
	unsigned int remote_length;            //length of the sockaddr_in structure
	int nbytes;                            //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];               //a buffer to store our received message

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	bzero(&server,sizeof(server));                  //zero the struct
	server.sin_family = AF_INET;                   //get server machine ip
	server.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	struct timeval tv;

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("unable to bind socket\n");
	}

	
	remote_length = sizeof(remote);

	while (1) {
		printf("\n******************* Server Listening *******************\n");
 		
 		bzero(buffer,sizeof(buffer));
	    struct packet client_pack;
	    tv.tv_sec = 200;
		tv.tv_usec = 0;

	    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    		printf("Error Socket timeout");
		}

	    if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, &remote_length) < 0)
	    {
	    	continue;
	    }
	    
	    printf("Command Received: %s\n", client_pack.command);

	    if (strcmp(client_pack.command,"put") == 0) {
	    	int packetExpected = 0;
	    	int flag = 0;
	    	int waitCount = 0;

	    	tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}
			int flagMD = 1;
			int flagPacketProcessed = 0;
	    	while (1) {
	    		printf("\n---------------------------------------\n");
	    		printf("Receive Packet number: %d\n", client_pack.seqNo);
	    		if (flagPacketProcessed == 0 && packetExpected == client_pack.seqNo) {

				    char filename[100];
				    strcpy(filename, "./serverDir/");
				    strcat(filename, client_pack.filename);

				    //Open file in write or append mode.
				    FILE *file;
				    if (packetExpected == 0) {
				    	file = fopen(filename,"wb");
				    } else {
				    	file = fopen(filename,"ab");
				   	}
				    int clientId = client_pack.clientId;						
				    unsigned char *file_buffer = client_pack.data;				

				    memcpy(file_buffer, client_pack.data, client_pack.dataSize);
				    int fileSize = fwrite(file_buffer , sizeof(unsigned char), client_pack.dataSize, file);

				  	printf("Packet: %d     bytes_wrote: %d\n", packetExpected, fileSize);

				    if( fileSize < 0)
				    {
				    	printf("error writting file\n");
				        exit(1);
				    }

				    //Construct ack packet.
				    struct packet sendPacket;
				    sendPacket.ack = 1 ;
				    sendPacket.seqNo = packetExpected;

					nbytes = sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
					flag = 1;
					flagPacketProcessed = 1;
					packetExpected++;
					fclose(file);
					//Breakout from the loop if all packets have been received.
					if (client_pack.seqNo == client_pack.totalPackets - 1) {
		    			client_pack.ack = 1 ;
						nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
						if (nbytes < 0){
							printf("Error in sendto\n");
						}
						printf("File Transfer successfully!!!\n");
						break;
					}
					//Re-transmit the packet if ack is not received.
				}  else if (packetExpected > client_pack.seqNo) {
					struct packet sendPacket;
				    sendPacket.ack = 1 ;
				    sendPacket.seqNo = packetExpected;

					nbytes = sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
				} else {
					if (flagPacketProcessed == 0) {
						printf("Received Packet number didn't match the expected one.\n Received %d  Expected: %d.\n", client_pack.seqNo, packetExpected);
					}	
				}
				if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, &remote_length) < 0)
	    		{
	    			printf("Waiting for packet.\n");
	    			waitCount++;
	    			//Wait fpr only 600 * 10 ms for each packet, else assume that server is down and client moves out from loop.
	    			if (waitCount > 10) {
	    				printf("Client Not sending packets. Moving on...\n");
	    				remove(client_pack.filename);
	    				break;
	    			}
	    			flagMD = 0;
	    			continue;
	    		} else {
	    			flagMD = 1;
	    			flagPacketProcessed = 0;
	    		}
			}
			printf("\nTotal Packets Received: %d\n", packetExpected-1);
			//The following code is what client does when get command is received.
		} else if (strcmp(client_pack.command,"get") == 0) {

			//set recvfrom timeout.
			tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;		//600ms

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
	    		printf("Error Socket timeout\n");
			}
			FILE *file;
			char filename[50];
			memset(filename, '\0', sizeof(filename));
			strcpy(filename, client_pack.filename);
			file = getFilePointer(filename);

			//Throw error if file doesn't exist.
			if(file == NULL)
		    {
		    	printf("Given File Name does not exist\n");
		    	struct packet pack;
			    pack.dataSize = -1;
		      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
				if (nbytes < 0){
					printf("Error in sendto\n");
				}
		      continue;
		    }

		    unsigned char file_buffer[FILEPACKETSIZE];
		  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
		  	fseek(file, 0, SEEK_SET);

		  	int count = 0;
		  	int totalPackets = getTotalNumberOfPackets(file_size);
		  	printf("Total Packets: %d\n", totalPackets);

		  	//Iterate over till the packet sent has a sequence Number equal to total packets to be sent.
		  	while (count < totalPackets) {
		  		printf("------------------------------------------------------------------------\n");
		  		bzero(file_buffer,sizeof(file_buffer));
		  		bzero(buffer,sizeof(buffer));
			  	int byte_read = fread(file_buffer, sizeof(unsigned char), FILEPACKETSIZE, file);		  	
			  	if( byte_read <= 0)
			    {
			        printf("unable to copy file into file_buffer\n");
			      	struct packet pack;
				    pack.dataSize = -1;
			      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
			      break;
			    }
			    //Construct the packet to be sent.
			    struct packet pack;
			    pack.clientId = client_pack.clientId;
			    memset(pack.filename, '\0', sizeof(pack.filename));
			    strcpy(pack.filename, filename);
				pack.dataSize = byte_read;
				pack.totalPackets = totalPackets;
				pack.seqNo = count;
				memset(pack.command, '\0', sizeof(pack.command));
			    strcpy(pack.command, "get");
			    memcpy(pack.data, file_buffer, sizeof(file_buffer));

			  	int resend_count = 0;
			  	// This is how reliability is implemented.
			  	RESEND_GET:
			  		printf("Packet: %d     bytes_read: %d\n", count, byte_read);
			  		
				    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

				    if (nbytes < 0){
						printf("Error in sendto\n");
					}
					printf("Waiting for client ack\n");
					struct packet receivedPacket;
					
					nbytes = recvfrom(sock, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&remote, &remote_length);  
					
					if (nbytes > 0 && receivedPacket.seqNo >= count && receivedPacket.ack == 1) {
						printf("Client Acknowledged Packet %d \n", count);
					} else {
						//Re-transmit the packet if ack is not received.
						if (resend_count < 10) {
							printf("Timer timeout, Resending packet %d\n", count);
							resend_count++;
							goto RESEND_GET;
						} else {
							printf("Resent 5 times, still client didn't acknowledge. Moving on...\n");
							break;
						}
					}
				count++;
			}

			if (count == 0) {
				printf("\nTotal Packets sent: %d   totalPackets: %d \n", count, totalPackets);
			} else {
				printf("\nTotal Packets sent: %d   totalPackets: %d \n", count - 1, totalPackets - 1);
			}
			//LS Command
		} else if (strcmp(client_pack.command, "ls") == 0) {
			tv.tv_sec = 0;
			tv.tv_usec = timeoutMicroseconds;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}
			DIR *dir;
			struct dirent *ent;
			struct packet pack;
			pack.clientId = client_pack.clientId;
			int fileNumber = 0;
			//Check if directory exist.
			if ((dir = opendir ("./serverDir/")) != NULL) {
			    while ((ent = readdir (dir)) != NULL) {
			    	//Add the file name only if its not . or ..
			    	if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 ) {
				    	if (fileNumber == 0) {
				    		strcpy((char *) pack.data, ent->d_name);
				    	} else {
				    		strcat((char *) pack.data, ent->d_name);
				    	}
				    	//Concatenate all filenames separated by #
				    	strcat((char *)pack.data, "#");

						fileNumber++;
					}
				}
				closedir (dir);

				//Construct the packet
				pack.dataSize = fileNumber;
				pack.totalPackets = 1;
				pack.seqNo = 1;
				memset(pack.command, '\0', sizeof(pack.command));
				strcpy(pack.command, "ls");
				int resend_count = 0;
				//Reliability
				RESEND_LS:
					nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

				    if (nbytes < 0) {
						printf("\nError in sendto\n");
					}
					
					struct packet receivedPacket;
					nbytes = recvfrom(sock, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&remote, &remote_length);  
					
					if (nbytes > 0 && receivedPacket.seqNo == 1 && receivedPacket.ack == 1) {
						printf("Client Acknowledged Packet\n");
					} else {
						//Resend packet if ack is not received, a maz of 10 times.
						if (resend_count < 10) {
							resend_count++;
							printf("Client Didn't ack the packet, Resending...\n");
							goto RESEND_LS;
						} else {
							printf("Resent 5 times, still client didn't acknowledge. Please try again later.\n");
						}
					}

			} else {
			    printf("Error while Opening Server Directory\n");
		      	struct packet pack;
			    pack.dataSize = -1;
			    //Send error to client
			    strcpy((char *)pack.data, "Error while Opening Server Directory");
		      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
				if (nbytes < 0){
					printf("Error in sendto\n");
				}
			}
		} else if (strcmp(client_pack.command, "del") == 0) {

			struct packet pack;
			pack.clientId = client_pack.clientId;
			int fileNumber = 0;

			char filename[100];
		    //memset(filename, '\0', sizeof(pac.filename));
		    strcpy(filename, "./serverDir/");
		    strcat(filename, client_pack.filename);

		    //Check to find if given filename exists in directory.
		    FILE *file = fopen(filename,"rb");
		    if (file) {
		    	fclose(file);
		    	//Delete the given filename.
		    	if (remove(filename) == 0) {
			      printf("Deleted successfully\n");
			      strcpy((char *) pack.data, "Deleted successfully");
		    	}
			   else {
			      printf("Unable to delete the file\n");
			      strcpy((char *) pack.data, "Unable to delete the file");
			   }
		    } else {
		    	printf("File Doesn't exist\n");
			    strcpy((char *) pack.data, "File Doesn't exist");
		    }
			
			pack.dataSize = fileNumber;
			pack.totalPackets = 1;
			pack.seqNo = 1;
			memset(pack.command, '\0', sizeof(pack.command));
			strcpy(pack.command, "del");
			int resend_count = 0;

			//Reliability
			RESEND_DEL:

				nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

			    if (nbytes < 0) {
					printf("\nError in sendto\n");
				}
				
				struct packet receivedPacket;
				nbytes = recvfrom(sock, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr *)&remote, &remote_length);  
				
				if (nbytes > 0 && receivedPacket.seqNo == 1 && receivedPacket.ack == 1) {
					printf("Client Acknowledged Packet\n");
				} else {
					//Resend a max 10 times if ack not received.
					if (resend_count < 10) {
						resend_count++;
						goto RESEND_DEL;
					} else {
						printf("Resent 5 times, still client didn't acknowledge. Please try again later.\n");
					}
				}
				// Exit gracefully
		} else if (strcmp(client_pack.command, "exit") == 0) {
			nbytes = sendto(sock, "Bye!", 6, 0, (struct sockaddr *)&remote, remote_length);
			if (nbytes < 0){
				printf("Error in sendto\n");
			}
			printf("Good Bye!\n");
			break;
			//Invalid command received.
		} else {
			bzero(client_pack.data, sizeof(client_pack.data));
			strcpy((char *) client_pack.data, "Invalid Command");

			nbytes = sendto(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);
			if (nbytes < 0){
				printf("Error in sendto\n");
			}
			//Clear screen.
			system("clear");
			printf("Invalid Command received\n");
		}
	}
	close(sock);
}
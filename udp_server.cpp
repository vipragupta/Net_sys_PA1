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
/* You will have to modify the program below */

#define MAXBUFSIZE 2000
#define FILEPACKETSIZE 500		//1048576 bytes or 10 MB

struct packet
{
	int clientId;
	char filename[100];
	unsigned char data[FILEPACKETSIZE];
	char command[50];
	int totalPackets;
	int seqNo;
	int dataSize;
};

size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
  	size_t file_size = ftell(file);
  	return file_size;
}

int getTotalNumberOfPackets(size_t file_size) {

	int packets = file_size/FILEPACKETSIZE;
  	if (file_size % FILEPACKETSIZE > 0) {
  		packets++;
  	}
  	return packets;
}

long unsigned int getBufferContentSize(unsigned char buffer[]) {
	long unsigned int buffSize = 0;

	while (buffer[buffSize] != '\0') {
		buffSize++;
	}
	return buffSize;
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
	tv.tv_sec = 2;
	tv.tv_usec = 0;

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
		printf("\n\n---------------------------------------Server Listening---------------------------------------\n");
		//File receive
 		
 		bzero(buffer,sizeof(buffer));
	    struct packet client_pack;
	    tv.tv_sec = 20;
		tv.tv_usec = 0;

	    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    		printf("Error Socket timeout");
		}

	    if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, &remote_length) < 0)
	    {
	    	continue;
	    }
	    
	    printf("Command Received: %s\n", client_pack.command);
	    //printf("FILENAME: %s\n", client_pack.filename);

	    if (strcmp(client_pack.command,"put") == 0) {
	    	int packetExpected = 0;
	    	int flag = 0;
	    	int waitCount = 0;

	    	tv.tv_sec = 0;
			tv.tv_usec = 400000;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}

	    	while (1) {
	    		if (packetExpected == client_pack.seqNo) {
				    char filename[100];
				    //memset(filename, '\0', sizeof(pac.filename));
				    strcpy(filename, "./serverDir/");
				    strcat(filename, client_pack.filename);

				    FILE *file;
				    if (packetExpected == 0) {
				    	file = fopen(filename,"wb");
				    } else {
				    	file = fopen(filename,"ab");
				   	}
				    int clientId = client_pack.clientId;						//getClientId(buffer);
				    unsigned char *file_buffer = client_pack.data;				//getFileContent(buffer);

				    memcpy(file_buffer, client_pack.data, client_pack.dataSize);
				    int fileSize = fwrite(file_buffer , sizeof(unsigned char), client_pack.dataSize, file);

				    printf("Received Packet : %s  %d\n", client_pack.filename, client_pack.seqNo);
				  	//printf("BUFFER:%s:\n\n", client_pack.data);

				    if( fileSize < 0)
				    {
				    	printf("error writting file\n");
				        exit(1);
				    }

					nbytes = sendto(sock, "Packet Received", 17, 0, (struct sockaddr *)&remote, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
					flag = 1;
					if (client_pack.seqNo == client_pack.totalPackets - 1) {
						break;
					}
					packetExpected++;
					fclose(file);
				}
				if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&remote, &remote_length)<0)
	    		{
	    			printf("error in recieving the file\n");
	    			waitCount++;
	    			if (waitCount > 20) {
	    				printf("Client Not sending packets. Moving on...\n");
	    				break;
	    			}
	    			continue;
	    		}
	    		if (client_pack.seqNo == (client_pack.totalPackets - 1)) {
	    			nbytes = sendto(sock, "Packet Received", 17, 0, (struct sockaddr *)&remote, remote_length);
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
					break;
				}
			}
			printf("Total Packets Received: %d\n", packetExpected);

		} else if (strcmp(client_pack.command,"get") == 0) {

			FILE *file;
			char filename[50];
			memset(filename, '\0', sizeof(filename));
			strcpy(filename, client_pack.filename);
			file = getFilePointer(filename);

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

		  	tv.tv_sec = 0;
			tv.tv_usec = 100000;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}

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

			    struct packet pack;
			    pack.clientId = client_pack.clientId;
			    memset(pack.filename, '\0', sizeof(pack.filename));
			    strcpy(pack.filename, filename);
				pack.dataSize = byte_read;//getBufferContentSize(file_buffer)-1;
				pack.totalPackets = totalPackets;
				pack.seqNo = count;
				memset(pack.command, '\0', sizeof(pack.command));
			    strcpy(pack.command, "put");
			    memcpy(pack.data, file_buffer, sizeof(file_buffer));

			    //printf("DATA SIZE:%d:\n", pack.dataSize);
			  	//printf("Buffer Content:%d  %lu   %lu  %lu\n", byte_read, sizeof(file_buffer), getBufferContentSize(file_buffer), getBufferContentSize(pack.data));
			  	//printf("BUFFER:%s:\n\n", pack.data);

			  	int resend_count =0;
			  	RESEND_GET:
			  		printf("Packet Sent: %s  %d\n", pack.filename, count);
				    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

				    if (nbytes < 0){
						printf("Error in sendto\n");
					}
					printf("Waiting for client ack\n");
					nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);  
					if (nbytes > 0) {
						printf("Client Says: %s\n", buffer);
					} else {
						if (count < 5) {
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

			nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);  
			if (nbytes > 0) {
				//printf("Client Says: %s\n", buffer);
			}
			printf("Total Packets sent: %d\n", count-1);
			tv.tv_sec = 20;
			tv.tv_usec = 100000;

		    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	    		printf("Error Socket timeout");
			}
		} else if (strcmp(client_pack.command, "ls") == 0) {

			DIR *dir;
			struct dirent *ent;
			struct packet pack;
			pack.clientId = client_pack.clientId;
			int fileNumber = 0;

			if ((dir = opendir ("./serverDir/")) != NULL) {
			    while ((ent = readdir (dir)) != NULL) {
			    	printf("%s\t", ent->d_name);

			    	if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 ) {
				    	if (fileNumber == 0) {
				    		strcpy((char *) pack.data, ent->d_name);
				    	} else {
				    		strcat((char *) pack.data, ent->d_name);
				    	}
				    	strcat((char *)pack.data, "#");

						fileNumber++;
					}
				}
				closedir (dir);
				pack.dataSize = fileNumber;
				pack.totalPackets = 1;
				pack.seqNo = 1;
				memset(pack.command, '\0', sizeof(pack.command));
				strcpy(pack.command, "ls");
				int resend_count = 0;

				RESEND_LS:
					nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

				    if (nbytes < 0) {
						printf("\nError in sendto\n");
					}
					
					nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);  
					if (nbytes > 0) {
						printf("\n\nClient Says: %s\n", buffer);
					} else {
						if (resend_count < 5) {
							resend_count++;
							goto RESEND_LS;
						} else {
							printf("Resent 5 times, still client didn't acknowledge. Please try again later.\n");
						}
					}

			} else {
			    printf("Error while Opening Server Directory\n");
		      	struct packet pack;
			    pack.dataSize = -1;
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

		    FILE *file = fopen(filename,"rb");
		    if (file) {
		    	fclose(file);
		    	if (remove(filename) == 0) {
			      printf("Deleted successfully");
			      strcpy((char *) pack.data, "Deleted successfully");
		    	}
			   else {
			      printf("Unable to delete the file");
			      strcpy((char *) pack.data, "Unable to delete the file");
			   }
		    } else {
		    	printf("File Doesn't exist");
			    strcpy((char *) pack.data, "File Doesn't exist");
		    }
			
			pack.dataSize = fileNumber;
			pack.totalPackets = 1;
			pack.seqNo = 1;
			memset(pack.command, '\0', sizeof(pack.command));
			strcpy(pack.command, "del");
			int resend_count = 0;

			RESEND_DEL:
				nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&remote, remote_length);

			    if (nbytes < 0) {
					printf("\nError in sendto\n");
				}
				
				nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);  
				if (nbytes > 0) {
					printf("\n\nClient Says: %s\n", buffer);
				} else {
					if (resend_count < 5) {
						resend_count++;
						goto RESEND_DEL;
					} else {
						printf("Resent 5 times, still client didn't acknowledge. Please try again later.\n");
					}
				}
		} else if (strcmp(client_pack.command, "exit") == 0) {
			nbytes = sendto(sock, "Bye!", 6, 0, (struct sockaddr *)&remote, remote_length);
			if (nbytes < 0){
				printf("Error in sendto\n");
			}
			printf("Good Bye!\n");
			break;
		} else {
			nbytes = sendto(sock, "Invalid Command\n", 17, 0, (struct sockaddr *)&remote, remote_length);
			if (nbytes < 0){
				printf("Error in sendto\n");
			}
			printf("Invalid Command received\n");
		}
	}
	close(sock);
}


		// Simple message receive.
		// bzero(buffer,sizeof(buffer));
		// nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
		
		// if (nbytes < 0){
		// 	printf("Error in recvfrom\n");
		// }
		// printf("The client says %s\n", buffer);
		// Simple message receive ends.
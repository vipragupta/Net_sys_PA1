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

#define MAXBUFSIZE 1048576		//1048576 bytes or 10 MB
#define FILEPACKETSIZE 500			// 10kB

/* You will have to modify the program below */

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
  	return packets -1;
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

long unsigned int getBufferContentSize(unsigned char buffer[]) {
	long unsigned int buffSize = 0;

	while (buffer[buffSize] != '\0') {
		buffSize++;
	}

	return buffSize+1;
}

FILE *getFilePointer(char filename[])
{
    FILE *file = NULL;
    char filePath[50];
   	memset(filePath, '\0', sizeof(filePath));
    strcat(filePath, "./clientDir/");
    strcat(filePath, filename)	;

    file = fopen(filePath, "rb");
    if (file) {
        printf("File %s opened \n\n", filePath);
        return file;
    }
    return NULL; // error
}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
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

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    	printf("Error Socket timeout");
	}

	while (1) {
		char option[100];
		char filename[50];
		bzero(option,sizeof(option));
		printf("\n\n\n********Menu********\n1. put\n2. get\n3. del\n4. ls\n5. exit\n\n");
		printf("Enter the operation you want to perform: ");
		scanf("%s", option);

		printf("You selected: %s\n", option);

		if (option[0] == '1') {

			printf("Enter the file Name: ");
			scanf("%s", filename);
			//Read a file.
			FILE *file;
			file = getFilePointer(filename);
			
			if(file == NULL)
		    {
		      printf("Given File Name does not exist\n");
		      continue;
		    }

		    printf("Getting file Size\n");
		  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
		  	printf("file Size: %lu\n", file_size);
		  	fseek(file, 0, SEEK_SET);

		  	int count = 0;
		  	int totalPackets = getTotalNumberOfPackets(file_size);
		  	printf("totalPackets: %d\n", totalPackets);

		  	while (count < totalPackets) {
		  		int retry = 0;
		  		printf("------------------------------------------------------------------------\n");
		  		
		  		bzero(file_buffer,sizeof(file_buffer));
		  		bzero(buffer,sizeof(buffer));
			  	int byte_read = fread(file_buffer, sizeof(unsigned char), FILEPACKETSIZE, file);		  	
			  	printf("Finally: %d   %d  %d\n", count, totalPackets, byte_read);
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

			    printf("DATA SIZE:%d:\n", pack.dataSize);
			  	printf("BUFFER:%s:\n\n", pack.data);

			  	RESEND:
				    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));

				    if (nbytes < 0){
						printf("Error in sendto\n");
					}

					nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &server_length);  
					if (nbytes > 0) {
						printf("Server Says: %s\n", buffer);
					} else {
						printf("Timer timeout, Resending packet %d\n", count);
						if (retry < 3) {
							retry++;
							goto RESEND;
						}
					}
				count++;
			}
			printf("Finally::: %d   %d\n", count, totalPackets);
		} else if (option[0] == '2') {
			char filename[100];
		    printf("Enter the file Name: ");
			scanf("%s", filename);
		    printf("\n");
		    struct packet pack;
		    pack.clientId = client;
		    memset(pack.filename, '\0', sizeof(pack.filename));
		    strcpy(pack.filename, filename);

			pack.seqNo = 0;
			memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "get");

		    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
		    printf("\n");
		    if (nbytes < 0){
				printf("Error in sendto\n");
			}

		  	bzero(file_buffer,sizeof(file_buffer));
		  	int packetExpected = 0;
		  	while (1) {
			  	struct packet client_pack;
			  	if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
			    {
			    	printf("error in recieving the file\n");
			    	continue;
			    }
			    
			    if (client_pack.dataSize == -1) {
			    	printf("Given File Name does not exist\n");
			    	break;
			    }
			    if (packetExpected == client_pack.seqNo) {
				    printf("Received something from server\n");
				    FILE *file;
				    char filePath[50];
				    memset(filePath, '\0', sizeof(filePath));
	    			strcat(filePath, "./clientDir/");
	    			strcat(filePath, filename)	;

				    if (packetExpected == 0) {
				    	file = fopen(filePath,"wb");
				    } else {
				    	file = fopen(filePath,"ab");
				   	}

				    unsigned char *file_buffer = client_pack.data;				//getFileContent(buffer);

				    memcpy(file_buffer, client_pack.data, client_pack.dataSize);
				    int fileSize = fwrite(file_buffer , sizeof(unsigned char), client_pack.dataSize, file);

				    printf("CLIENT ID:%d:\n", client_pack.clientId);
				    printf("DATA SIZE:%d:\n", client_pack.dataSize);
				  	printf("Buffer Content:%d  %lu   %lu  %lu\n", fileSize, sizeof(file_buffer), getBufferContentSize(file_buffer), getBufferContentSize(client_pack.data));
				  	printf("BUFFER:%s:\n\n", client_pack.data);

				    if( fileSize < 0)
				    {
				    	printf("error writting file\n");
				        exit(1);
				    }

					nbytes = sendto(sock, "Packet Received\n", 17, 0, (struct sockaddr *)&server, sizeof(server));
					if (nbytes < 0){
						printf("Error in sendto\n");
					}
					fclose(file);
					if (client_pack.seqNo == client_pack.totalPackets - 1) {
						nbytes = sendto(sock, "Packet Received\n", 17, 0, (struct sockaddr *)&server, sizeof(server));
						if (nbytes < 0){
							printf("Error in sendto\n");
						}
						break;
					}
					packetExpected++;
				}
			}
		} else if (option[0] == '3') {
			

		} else if (option[0] == '4') {
			printf("\n");
			struct packet pack;
		    pack.clientId = client;

			pack.seqNo = 0;
			memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "ls");

		    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
		    if (nbytes < 0){
				printf("Error in sendto\n");
			}

		  	bzero(file_buffer,sizeof(file_buffer));
		  	struct packet client_pack;
		  	if (recvfrom(sock, &client_pack, sizeof(packet), 0, (struct sockaddr *)&server, &server_length) < 0)
		    {
		    	printf("error in recieving the file\n");
		    	continue;
		    }
		    if (client_pack.dataSize == -1) {
				printf("Some error occured at server side.\n");
			} else {

				nbytes = sendto(sock, "Packet Received", 17, 0, (struct sockaddr *)&server, sizeof(server));
			    if (nbytes < 0){
					printf("Error in sendto\n");
				}

				char *token;
				int tcount = 0;
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
			}

		} else if (option[0] == '5') {
			struct packet pack;
		    memset(pack.command, '\0', sizeof(pack.command));
		    strcpy(pack.command, "exit");
		    RESEND_EXIT:
		      	nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));
				if (nbytes < 0){
					printf("Error in sendto\n");
					goto RESEND_EXIT;
				}
			printf("Good Bye!\n");
			return 0;
		} else {
			printf("The option you selected is invalid.\n");
			continue;
		}
	
	}
	close(sock);
}

/*Simple message sending
		char command[] = "apple";	
		nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&server, sizeof(server));

		if (nbytes < 0){
			printf("Error in sendto\n");
		}
		bzero(buffer,sizeof(buffer));
		Simple message sending ends.
		*/
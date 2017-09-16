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

#define MAXBUFSIZE 1048576		//1048576 bytes or 10 MB
#define FILEPACKETSIZE 500			// 10kB

/* You will have to modify the program below */

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

char *getClientID() {
	srand (time(NULL));
  	char *clientId;
  	clientId = (char *) malloc(9);
  	int i = 0;
  	while(i<4){
  		clientId[i] = (char)(rand() % 10 + 64);
  		i++;
  	}
  	clientId[i++] = '#';
  	clientId[i++] = '#';
  	clientId[i++] = '#';
  	clientId[i++] = '#';
  	clientId[i++] = '#';
  	clientId[i++] = '#';
  	return clientId;
}

char *getPacketHeader(char *clientId, int seqNo, int totalPackets, char *fileName, char *cmd) {
	char header[MAXBUFSIZE];

}

long unsigned int getBufferContentSize(char buffer[]) {
	long unsigned int buffSize = 0;

	while (buffer[buffSize] != '\0') {
		buffSize++;
	}

	return buffSize+1;
}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char file_buffer[FILEPACKETSIZE];
	char *clientId = getClientID();
	struct sockaddr_in server;              //"Internet socket address structure"

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
	
	//Read a file.
	FILE *file;
	file = fopen("foo1", "r");
	if(file == NULL)
    {
      printf("file does not exist\n");
    }

  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
  	printf("file_size: %lu\n\n", file_size);

  	fseek(file, 0, SEEK_SET);

  	int count = 0;
  	int totalPackets = getTotalNumberOfPackets(file_size);

  	while (count < 3) {

	  	int byte_read = fread(file_buffer, 1, FILEPACKETSIZE, file);
	  	if( byte_read <= 0)
	    {
	      printf("unable to copy file into file_buffer\n");
	      exit(1);
	    }

	  	strcpy(buffer, clientId);     
	  	strcat(buffer, file_buffer);
	  	printf("Buffer Content:%lu\n", getBufferContentSize(file_buffer));
	  	printf("BUFFER: \n:%s:\n\n", buffer);

	    nbytes = sendto(sock, buffer, sizeof(clientId) + getBufferContentSize(file_buffer), 0, (struct sockaddr *)&server, sizeof(server));

	    if (nbytes < 0){
			printf("Error in sendto\n");
		}

	  	bzero(file_buffer,sizeof(file_buffer));
	  	bzero(buffer,sizeof(buffer));
	    //Read a file ends
	
		unsigned int server_length = sizeof(server);
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &server_length);  

		printf("Server says %s\n", buffer);
		count++;
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
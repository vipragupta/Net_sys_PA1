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
	unsigned char data[FILEPACKETSIZE];
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

int getClientID() {
	srand (time(NULL));
  	int clientId = 0;
  	int i = 0;
  	while(i<4){
  		clientId = clientId * pow(10, i) + (char)(rand() % 10 + 1);
  		i++;
  	}
  	printf("REAL CLIENTID%d\n", clientId);
  	return clientId;
}

long unsigned int getBufferContentSize(unsigned char buffer[]) {
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
	char buffer[FILEPACKETSIZE];
	unsigned char file_buffer[FILEPACKETSIZE];
	int client = getClientID();
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
	file = fopen("foo2", "r");
	if(file == NULL)
    {
      printf("file does not exist\n");
    }

  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
  	printf("file_size: %lu\n\n", file_size);

  	fseek(file, 0, SEEK_SET);

  	int count = 0;
  	int totalPackets = getTotalNumberOfPackets(file_size);

  	while (count < totalPackets) {


  		printf("------------------------------------------------------------------------\n");
	  	int byte_read = fread(file_buffer, sizeof(unsigned char), FILEPACKETSIZE, file);
	  	if( byte_read <= 0)
	    {
	      printf("unable to copy file into file_buffer\n");
	      exit(1);
	    }

	    struct packet pack;
	    pack.clientId = client;
		pack.dataSize = byte_read;//getBufferContentSize(file_buffer)-1;

	    memcpy(pack.data, file_buffer, sizeof(file_buffer));

	    printf("CLIENT ID:%d**%d:\n\n", client, pack.clientId);
	    printf("DATA SIZE:%d:\n", pack.dataSize);
	  	printf("Buffer Content:%d  %lu   %lu  %lu\n", byte_read, sizeof(file_buffer), getBufferContentSize(file_buffer), getBufferContentSize(pack.data));
	  	printf("BUFFER:%s:\n\n", pack.data);

	    nbytes = sendto(sock, &pack, sizeof(packet), 0, (struct sockaddr *)&server, sizeof(server));

	    if (nbytes < 0){
			printf("Error in sendto\n");
		}

	  	bzero(file_buffer,sizeof(file_buffer));
	    //Read a file ends
	
		unsigned int server_length = sizeof(server);
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &server_length);  

		//printf("Server says %s\n", buffer);
		count++;
		printf("------------------------------------------------------------------------\n\n");
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
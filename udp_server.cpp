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
/* You will have to modify the program below */

#define MAXBUFSIZE 2000
#define MAXFILEBUFFSIZE 500		//1048576 bytes or 10 MB

long unsigned int getBufferContentSize(char buffer[]) {
	long unsigned int buffSize = 0;

	while (buffer[buffSize] != '\0') {
		buffSize++;
	}

	return buffSize+1;
}

long unsigned int getStartingIndex(char buffer[], int val) {
	long unsigned int buffSize = 0;
	int count = 0;
	int flag = 0;

	while (buffer[buffSize] != '\0') {
		//printf("BUFFER CONTENT:  %c\n", buffer[buffSize]);
	    if (buffer[buffSize] == '#') {
	    	count++;
	    	if (count == val) {
	    		flag = 1;
	    		break;
	    	}
	    }
	    buffSize++;
	}
	//printf("FLAG: %d\n", flag);
	//printf("RETURNING: %lu\n", buffSize);

	if (flag == 1) {
		return buffSize;
	} else {
		return -1;
	}
}

char *getClientId(char buffer[]) {
	long unsigned int buffSize = getStartingIndex(buffer, 1);
	char *clientId = (char *) malloc(sizeof(char) * buffSize);
  	int i=0;
	for (; i < buffSize; i++) {
		clientId[i] = buffer[i];
	}
	clientId[i] = '\0';
	return clientId;
}

char *getFileContent(char buffer[]) {
	long unsigned int buffSize = getStartingIndex(buffer, 6);

	char *file_buffer = (char *) malloc(MAXFILEBUFFSIZE+1);
	file_buffer[0] = '\0';
  	buffSize++;
  	int i=0;
  	printf("^^^Initial FILEBUFFER\n%s\n\n", file_buffer);
	if (buffSize != -1) {	
		while (buffer[buffSize] != '\0') {
			file_buffer[i++] = buffer[buffSize++];
		}
	}
	file_buffer[i] = '\0';
	return file_buffer;
}


int main (int argc, char * argv[] )
{
	int sock;                              //This will be our socket
	struct sockaddr_in server, remote;     //"Internet socket address structure"
	unsigned int remote_length;            //length of the sockaddr_in structure
	int nbytes;                            //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];               //a buffer to store our received message
	FILE *file;

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	bzero(&server,sizeof(server));                  //zero the struct
	server.sin_family = AF_INET;                   //get server machine ip
	server.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


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
		//File receive
 		
 		bzero(buffer,sizeof(buffer));
 		printf("Initial buffer: \n:%s:\n\n\n\n", buffer);

	    if (recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length)<0)
	    {
	    	printf("error in recieving the file\n");
	    	continue;
	    }
	    printf("buffer: %lu  %lu:\n\n", sizeof(buffer), getBufferContentSize(buffer));
	    printf("buffer: \n:%s:\n\n\n\n", buffer);
	    file = fopen("foo1_new","ab");
	    char *clientId = getClientId(buffer);
	    char *file_buffer = getFileContent(buffer);

	    printf("\n\nclientId:%s\n\n", clientId);
	    

	    int fileSize = fwrite(file_buffer , 1, getBufferContentSize(file_buffer)-1, file);
	    //int fileSize = write(file, file_buffer, sizeof(file_buffer));

	    printf("Size of File received:%d  %lu\n", fileSize, getBufferContentSize(file_buffer));

	    if( fileSize < 0)
	    {
	    	printf("error writting file\n");
	        exit(1);
	    }

		nbytes = sendto(sock, "file received\n", 17, 0, (struct sockaddr *)&remote, remote_length);
		if (nbytes < 0){
			printf("Error in sendto\n");
		}
		fclose(file);
	
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
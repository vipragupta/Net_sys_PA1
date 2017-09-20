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
#define FILEPACKETSIZE 500		//1048576 bytes or 10 MB

struct packet
{
	int clientId;
	char filename[100];
	unsigned char data[FILEPACKETSIZE];
	int dataSize;
};

long unsigned int getBufferContentSize(unsigned char buffer[]) {
	long unsigned int buffSize = 0;

	while (buffer[buffSize] != '\0') {
		//printf(":%c:", buffer[buffSize]);
		buffSize++;
	}
	printf("\n%lu    \n\n", buffSize);
	return buffSize;
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

	FILE *file;

	/*char test[10] = "vipra";
	file = fopen("foo_test","ab");
	int fileSize = fwrite(test , 1, sizeof(test), file);
	fclose(file);
	*/

	while (1) {
		printf("------------------------------------------------------------------------\n");
		//File receive
 		
 		bzero(buffer,sizeof(buffer));
	    struct packet pac;

	    if (recvfrom(sock, &pac, sizeof(packet), 0, (struct sockaddr *)&remote, &remote_length)<0)
	    {
	    	printf("error in recieving the file\n");
	    	continue;
	    }

	    char filename[100];
	    //memset(filename, '\0', sizeof(pac.filename));
	    strcpy(filename, "./serverDir/");
	    strcat(filename, pac.filename);
	    strcat(filename, "_new");

	    printf("FILENAME: %s   %s\n", filename, pac.filename);
	    file = fopen(filename,"ab");
	   
	    int clientId = pac.clientId;						//getClientId(buffer);
	    unsigned char *file_buffer = pac.data;				//getFileContent(buffer);

	    memcpy(file_buffer, pac.data, pac.dataSize);
	    int fileSize = fwrite(file_buffer , sizeof(unsigned char), pac.dataSize, file);

	    printf("CLIENT ID:%d:\n", pac.clientId);
	    printf("DATA SIZE:%d:\n", pac.dataSize);
	  	printf("Buffer Content:%d  %lu   %lu  %lu\n", fileSize, sizeof(file_buffer), getBufferContentSize(file_buffer), getBufferContentSize(pac.data));
	  	printf("BUFFER:%s:\n\n", pac.data);

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
		printf("------------------------------------------------------------------------\n\n");
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
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

#define MAXBUFSIZE 100
#define MAXFILEBUFFSIZE 104857		//1048576 bytes or 10 MB

int main (int argc, char * argv[] )
{


	int sock;                              //This will be our socket
	struct sockaddr_in server, remote;     //"Internet socket address structure"
	unsigned int remote_length;            //length of the sockaddr_in structure
	int nbytes;                            //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];               //a buffer to store our received message
	char file_buffer[MAXFILEBUFFSIZE];

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server,sizeof(server));                  //zero the struct
	server.sin_family = AF_INET;                   //get server machine ip
	server.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	while (1) {
		//waits for an incoming message

		// Simple message receive.
		// bzero(buffer,sizeof(buffer));
		// nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
		
		// if (nbytes < 0){
		// 	printf("Error in recvfrom\n");
		// }
		// printf("The client says %s\n", buffer);
		// Simple message receive ends.

		//File receive
 
	    if (recvfrom(sock, file_buffer, MAXFILEBUFFSIZE, 0, (struct sockaddr *)&remote, &remote_length)<0)
	    {
	    	printf("error in recieving the file\n");
	    	continue;
	    }

	    printf("First byte is: %x\n", file_buffer[0]);

	    FILE *file;
	    file = fopen("foo2_new","w+");
	    
	    int fileSize = fwrite(file_buffer , 1, sizeof(file_buffer), file);

	    printf("Size of File received:%d\n", fileSize);

	    if( fileSize < 0)
	    {
	    	printf("error writting file\n");
	        exit(1);
	    }
	    //printf("The client says %s\n", file_buffer);
		//File receive ends.

		nbytes = sendto(sock, "file received\n", 17, 0, (struct sockaddr *)&remote, remote_length);
		if (nbytes < 0){
			printf("Error in sendto\n");
		}
	}
	close(sock);
}


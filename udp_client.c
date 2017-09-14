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

#define MAXBUFSIZE 100
#define MAXFILEBUFFSIZE 1048576		//1048576 bytes or 10 MB

/* You will have to modify the program below */

size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
  	size_t file_size = ftell(file);
  	return file_size;
}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char file_buffer[MAXFILEBUFFSIZE];

	struct sockaddr_in server;              //"Internet socket address structure"

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&server,sizeof(server));               //zero the struct
	server.sin_family = AF_INET;                 //address family
	server.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	server.sin_addr.s_addr = inet_addr(argv[1]); //sets server IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	
	//Read a file.
	FILE *file;
	file = fopen("foo2", "r");
	if(file == NULL)
    {
      printf("file does not exist\n");
    }

  	size_t file_size = getFileSize(file); 		//Tells the file size in bytes.
  	printf("file_size: %lu\n", file_size);

  	fseek(file, 0, SEEK_SET);

  	/*
	size_t fread(void *buffer, size_t element_size, size_t elements, FILE *file)
	buffer      : buffer to read into
	element_size: size of each element i.e. size of each element in bytes
	elements    : number of elements of specified size to be read.
	file 	 	: file to read the bytes from. 

	it returns the number of elements sucessfully read.
  	*/

  	int byte_read = fread(file_buffer, 1, file_size, file);

  	printf("First byte is: %x\n", file_buffer[0]);

  	if( byte_read <= 0)
    {
      printf("unable to copy file into buffer\n");
      exit(1);
    }

    nbytes = sendto(sock, file_buffer, file_size, 0, (struct sockaddr *)&server, sizeof(server));

    if (nbytes < 0){
		printf("Error in sendto\n");
	}

  	bzero(file_buffer,sizeof(file_buffer));
    //Read a file ends


  	/*Simple message sending
	char command[] = "apple";	
	nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&server, sizeof(server));

	if (nbytes < 0){
		printf("Error in sendto\n");
	}
	bzero(buffer,sizeof(buffer));
	Simple message sending ends.
	*/

	
	unsigned int server_length = sizeof(server);
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &server_length);  

	printf("Server says %s\n", buffer);

	close(sock);

}


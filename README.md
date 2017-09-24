# nsao

The code contains 2 files udp_client.cpp and udp_server.cpp. These are the 2 main files containing client and server code.

*Functionalities

1. put [FileName]  -- send file from client to server
2. get [FileName]  -- receive file from server to client
3. del [FileName]  -- deletes the file from serer directory if it exists
4. ls              -- get the list of files that server stores
5. exit 		   -- gracefully terminate client and server process

If user enters any other command except the above 5, Client sends it to server, server just responds back with same command saying 'invalid command'. There is another command that I have included "clear" to clear the console of client and server.

For packet transfer, I have used a struct which has the following structure:
struct packet
{
	int clientId;
	char filename[100];
	unsigned char data[FILEPACKETSIZE];
	char command[50];
	int totalPackets;
	int seqNo;
	int dataSize;
	char mdHash[FILEPACKETSIZE];
	int ack;
};

where FILEPACKETSIZE is 5*1024 i.e. 5 KB.

*Reliability

I have implemented stop and wait Reliability protocol. After sending a packet, the sender starts a for 400-600ms to receive the acknowledgement from server. If the timer times out, sender resends the packet and restarts the times. This is done 10 times before sender assumes that receiver is down and stops sending anymore data. Sender make sures that packet received from receiver is acknowledgement for same packet by checking the sequence id and ack flag in the packet received.
This reliability is added to all the commands.


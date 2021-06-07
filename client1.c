//Client side code for single server multiple clients
//Compile as : gcc 180001064_client.c -o client
//Execute after server as started as : ./client CLIENTPORTNUMBER
//Change the IP address used in both codes to your system's IP address
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define SERV_PORT 11000 //server port - same as in server code
#define STDIN 0 // stdin fd is 0

int main(int argc, char *argv[])
{
	// buffer are for message communication
	char inputBuffer[256], outputBuffer[256], desnPortBuf[256];
	struct sockaddr_in server_addr, client_addr;
	//set of socket descriptors
	fd_set readfds;

	if(argc!=2){ // require client port number as argument
		printf("Pass Client Port Number as an argument.");
		exit(0);
	}
	int clientPort = atoi(argv[1]);

	int clientfd = socket(AF_INET, SOCK_STREAM, 0); // create client socket
	if (clientfd < 0)
		printf("Error in client creation\n");
	else
		printf("Client Created successfully.\n");
		
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_addr.s_addr = inet_addr("sysIP"); // enter system's machine IP
	server_addr.sin_port = htons(SERV_PORT);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_addr.s_addr = inet_addr("sysIP"); // enter system's machine IP
	client_addr.sin_port = htons(clientPort); // port number is assigned after input from user
	
	if (bind(clientfd, (struct sockaddr*) &client_addr, sizeof(struct sockaddr_in)) == 0)
		printf("Port number binded Correctly\n");
	else
		printf("Unable to bind. Port number is either in use or inacesible.\n");
	
	int con = connect(clientfd, (struct sockaddr*) &server_addr, sizeof server_addr);
	if (con == 0)
		printf("Client Connected successfully\n");
	else
		printf("Error in Connection with server\n");
	
	recv(clientfd, outputBuffer, 256, 0);
	printf("Server Welcome Message: %s", outputBuffer);
	printf("Enter message at any time to send to other client.\n\n");
	
	while(1){

		memset(inputBuffer, 0, sizeof inputBuffer); // resetting all buffers
		memset(desnPortBuf, 0, sizeof desnPortBuf);
		memset(outputBuffer, 0, sizeof outputBuffer);

		FD_ZERO(&readfds);
        FD_SET(clientfd, &readfds);
        FD_SET(STDIN, &readfds);
		int max_sd = clientfd; //Always > 0

		printf("Waiting for some activity.\n\n");
		//wait indefinitely for activity on one of the sockets
		int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
	
		if ((activity < 0) && (errno!=EINTR))
			printf("error in selecting activity.\n");
			
        if (FD_ISSET(STDIN, &readfds)){ // if stdin input is detected
			fgets(inputBuffer, 256, stdin);
			printf("Enter Destination port number.\n");
			fgets(desnPortBuf, 256, stdin);
			inputBuffer[strcspn(inputBuffer, "\n")] = 0; //removing \n from end of line
			desnPortBuf[strcspn(desnPortBuf, "\n")] = 0;
			send(clientfd, inputBuffer, 256, 0);
			send(clientfd, desnPortBuf, 256, 0);
        }
        else if (FD_ISSET(clientfd, &readfds)){ // if message recieved from server
			recv(clientfd, outputBuffer, 256, 0);
			printf("Server sent : %s", outputBuffer);
        }
	}

	close(clientfd);
	return 0;
}


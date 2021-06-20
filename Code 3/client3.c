//Client side code for single server multiple clients
//Clients can pass messages or transfer files.
//Five commands are defined: list, subscribe, news, close, clear
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
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO macros

#define SERV_PORT 10000 //server port - same as in server code
#define STDIN 0 // stdin fd is 0
#define dataSize 256 // size of all buffer set

// function to recieve tpopic list
void recieve_list(int sockfd){
	char buffer[dataSize];
	while(1){ // keep reading until it reaches end of topics
		read(sockfd, buffer, dataSize); // read data from socket
		if(!strcmp(buffer, "END")) // if server sends END, it indicates end of list
			break;
		printf("%s\n", buffer); // print topic list
		bzero(buffer, dataSize); // clear buffer
	}
	return;
}

int main(int argc, char *argv[])
{
	// buffer are for message communication
	char userCmnd[dataSize], userTopic[dataSize], userNews[dataSize], stdErr[dataSize],
		serverCmnd[dataSize], serverTopic[dataSize], serverNews[dataSize], clientPortNo[dataSize];
	struct sockaddr_in server_addr, client_addr;
	//set of socket descriptors
	fd_set readfds;

	if(argc!=2){ // require client port number as argument
		printf("[-]Pass Client Port Number as an argument.");
		exit(0);
	}
	int clientPort = atoi(argv[1]);

	int clientfd = socket(AF_INET, SOCK_STREAM, 0); // create client socket
	if (clientfd < 0)
		printf("[-]Error in client creation.\n");
	else
		printf("[+]Client Created successfully.\n");
		
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_addr.s_addr = inet_addr("sysIP"); // enter system's machine IP
	server_addr.sin_port = htons(SERV_PORT);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_addr.s_addr = inet_addr("sysIP"); // enter system's machine IP
	client_addr.sin_port = htons(clientPort); // port number is assigned after input from user
	
	// assigning user input port number to the client
	if (bind(clientfd, (struct sockaddr*) &client_addr, sizeof(struct sockaddr_in)) == 0)
		printf("[+]Port number binded Correctly.\n");
	else{
		printf("[-]Unable to bind. Port number is either in use or inacesible.\n");
		printf("[+]Random port number allotted. Check server for info.\n");
	}

	// connecting to the server
	int con = connect(clientfd, (struct sockaddr*) &server_addr, sizeof server_addr);
	if (con == 0)
		printf("[+]Client Connected successfully to the server.\n");
	else
		printf("[-]Error in Connection with server.\n");
	
	read(clientfd, serverCmnd, dataSize); // welcome msg from the server
	printf("[+]Server Welcome Message: %s\n", serverCmnd);
	printf("[..]Enter message at any time to send to other client.\n");

	read(clientfd, clientPortNo, dataSize); // port number of client
	printf("[+]Port number assigned is: %s.\n", clientPortNo);

	
	while(1){

		memset(userCmnd, 0, dataSize); // resetting all buffers
		memset(userTopic, 0, dataSize);	
		memset(userNews, 0, dataSize);
		memset(stdErr, 0, dataSize);
		memset(serverCmnd, 0, dataSize);	
		memset(serverTopic, 0, dataSize);
		memset(serverNews, 0, dataSize);

		FD_ZERO(&readfds); // clear the socket set
        FD_SET(clientfd, &readfds); // add client socket to the set
        FD_SET(STDIN, &readfds); // add STDIN to the set
		int max_sd = clientfd; //Always > 0

		printf("\n[..]Waiting for some activity.\n");
		//wait indefinitely for activity on one of the sockets
		int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
	
		if ((activity < 0) && (errno!=EINTR))
			printf("[-]Error in selecting activity.\n");
			
        if (FD_ISSET(STDIN, &readfds)){ // if stdin input is detected
			fgets(userCmnd, dataSize, stdin); // first get command name
			userCmnd[strcspn(userCmnd, "\n")] = 0; //removing \n from end of line
			if(!strcmp(userCmnd, "list")){ // if user wants to recieve topic list
			}else if(!strcmp(userCmnd, "subscribe")){ // if user wants to subscribe to a topic
				printf("[..]Enter Topic name\n");
				fgets(userTopic, dataSize, stdin); // subscribe command requires name of topic
				userTopic[strcspn(userTopic, "\n")] = 0; //removing \n from end of line
			}else if(!strcmp(userCmnd, "news")){ // sending news on a topic
				printf("[..]Enter Topic\n");
				fgets(userTopic, dataSize, stdin); // topic name to be sent must be entered
				userTopic[strcspn(userTopic, "\n")] = 0; //removing \n from end of line
				printf("[..]Enter News\n");
				fgets(userNews, dataSize, stdin); // news to be sent must be entered
				userNews[strcspn(userNews, "\n")] = 0; //removing \n from end of line
			}else if(!strcmp(userCmnd, "close")){
				printf("[+]Closing client....\n");
				break;
			}else if(!strcmp(userCmnd, "clear")){ // cllearing the console
				system("clear");
				continue;
			}else{
				printf("[-]FTP Commands allowed:\na. list\nb. subscribe\nc. close\nd. news\ne. clear\n");
				continue;
			}

			send(clientfd, userCmnd, dataSize, 0); // sending data to server

			if(!strcmp(userCmnd, "subscribe") || !strcmp(userCmnd, "news"))
				send(clientfd, userTopic, dataSize, 0); // send additional data
			if(!strcmp(userCmnd, "news"))
				send(clientfd, userNews, dataSize, 0); // send additional data
        }

        else if (FD_ISSET(clientfd, &readfds)){ // if message recieved from server
			read(clientfd, serverCmnd, dataSize); // read command name
			if(!strcmp(serverCmnd, "list")){
				printf("[:]Server is listing available topics:\n");
				recieve_list(clientfd); // recieve topic list
			}else if(!strcmp(serverCmnd, "news")){ // is server sends news update
				read(clientfd, serverTopic, dataSize); // read topic name
				read(clientfd, serverNews, dataSize); // read news
				printf("[+]Server has sent a news for the topic - %s. News is - %s\n", serverTopic, serverNews);
			}else if(!strcmp(serverCmnd, "ERROR")){ // incase of error
				read(clientfd, stdErr, dataSize);
				printf("[-]Server said: %s\n", stdErr);
			}else if(!strcmp(serverCmnd, "refresh")) // incase server has recieved a new topic name
				printf("[+]List updated. Kindly request list again.\n");
			else
				printf("[-]Invalid command. Ignoring.\n");
        }
	}

	close(clientfd); // close the client
	return 0;
}
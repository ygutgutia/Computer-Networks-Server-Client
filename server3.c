// server side code for single server multiple clients
// compile as : gcc 180001064_server.c -o server
// execute before clients as : ./server
// acts as an intermediary for msg or file passing
// change the IP address used in both codes to your system's IP address

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdbool.h>  
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO macros
	
#define PORT 10000 // server Port number is fixed
#define max_clients 10 // can be changed
#define dataSize 256 // size of all buffer set
#define STDIN 0 // taking user input from console

int maxTopicCount = 10; // maximum topics server can store
int currTopicCount = 4; // current number of topics present
char topics[4][dataSize] = {"sports", "education", "entertainment", "business"};
bool clientSubscribe[4][max_clients]; // array stores which client is subscribed to which topic 

// function to send topic list to server
int send_list(int sockfd){
	char temp[dataSize] = {0};
	int i = 0;
	while(i < currTopicCount){ // sending each element of array
		strcpy(temp, topics[i]);
		send(sockfd, temp, dataSize, 0);
		bzero(temp, dataSize); // clear buffer
		i++;
	}
	send(sockfd, "END", dataSize, 0); // mark end of list
	return 1;
}	

int main(int argc, char *argv[])
{
	int serverSocket_fd, new_socket, client_socket[max_clients], valread;
	struct sockaddr_in address;
	// buffers to store client messages
	char commandName[dataSize], clientNews[dataSize], clientTopic[dataSize], rqstPortNo[dataSize], stdErr[dataSize];
	fd_set readfds; // set of socket descriptors
	char *message = "Server connected. Chat Enabled with other clients.\n"; // welcome msg for clients
	
	// initialise all client_socket[] to 0
	for (int i = 0; i < max_clients; i++)
		client_socket[i] = 0;
	
	// create a server socket
	if( (serverSocket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("[-]socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	// type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
		
	// bind the socket to a defined port number
	if (bind(serverSocket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("[-]binding socket to port number failed.");
		exit(EXIT_FAILURE);
	}
	printf("[+]Listening on port %d \n", PORT);
		
	// maximum 3 pending connections allowed
	if (listen(serverSocket_fd, 3) < 0){
		perror("[-]connection pending rqst overload");
		exit(EXIT_FAILURE);
	}
		
	// accept the incoming connection
	int addrlen = sizeof(address);
	puts("[+]Waiting for connections ...");

		
	while(1){

		FD_ZERO(&readfds); // clear the socket set
		FD_SET(serverSocket_fd, &readfds); // add master socket to set
        FD_SET(STDIN, &readfds); // add STDIN to the set
		int max_sd = serverSocket_fd;

		memset(commandName, 0, dataSize); // resetting the buffers
		memset(clientNews, 0, dataSize);
		memset(clientTopic, 0, dataSize);
		memset(rqstPortNo, 0, dataSize);
		memset(stdErr, 0, dataSize);
			
		//add child sockets to set
		for (int i = 0 ; i < max_clients ; i++){
			int sd = client_socket[i]; // socket descriptor
			// if socket descriptor is valid then we add to list
			if(sd > 0)
				FD_SET(sd, &readfds);
			// keeps count of highest fd number
			if(sd > max_sd)
				max_sd = sd;
		}
	
		printf("\n[..]Waiting for some activity.\n");
		//wait indefinitely for activity on one of the sockets
		int activity = select( max_sd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno!=EINTR))
			printf("[-]error in selecting activity.\n");
			

		if (FD_ISSET(STDIN, &readfds)){ // if stdin input is detected
			fgets(commandName, dataSize, stdin); // first get command name
			commandName[strcspn(commandName, "\n")] = 0; //removing \n from end of line
			if(!strcmp(commandName, "add")){ // if user wants to add topic name
				if(currTopicCount == maxTopicCount) // if topic list is already full
					printf("Topic list full. Try later.\n");
				else{
					printf("Enter new topic name.\n");
					fgets(clientTopic, dataSize, stdin); // first get command name
					clientTopic[strcspn(clientTopic, "\n")] = 0; //removing \n from end of line
					bool flag = false;
					for(int i=0; i<currTopicCount; i++){
						if(!strcmp(topics[i], clientTopic)){ // comparison is case sensitive
							printf("Topic already present.\n"); // check if entered topic is already present
							flag = true;
							break;
						}
					}
					if(!flag){ // if topic is new
						strcpy(topics[currTopicCount++], clientTopic);
						for (int i = 0 ; i < max_clients ; i++){
							if(client_socket[i] > 0){ // inform all current clients
								send(client_socket[i], "refresh", dataSize, 0);
							}
						}
						printf("Topics updated. Clients Informed.\n");
					}
				}
			}else if(!strcmp(commandName, "clear")){ // if user command is to clear the conssole
				system("clear");
				printf("[+]Screen cleared. Server running...\n");
			}else // if illegal command is entered
				printf("[-]Server Commands allowed:\na. add\nb. clear\n");

			continue;
        }
		
		// incoming coonnection request
		else if (FD_ISSET(serverSocket_fd, &readfds)){

			if ((new_socket = accept(serverSocket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
				perror("[-]Error in socket creation of client");
				exit(EXIT_FAILURE);
			}
			printf("[+]New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
		
			//send new connection greeting message
			if(send(new_socket, message, dataSize, 0) <= 0)
				perror("[-]Error sending welcome message");
			puts("[+]Welcome message sent successfully.");
			
			char tempClientPortNo[dataSize];
  			sprintf(tempClientPortNo, "%d", ntohs(address.sin_port));
			send(new_socket, tempClientPortNo, dataSize, 0); // sending port number of client
				
			//add new socket to array of sockets
			for (int i = 0; i < max_clients; i++){
				if( client_socket[i] == 0 ){
					client_socket[i] = new_socket;
					printf("[+]Added client to list of sockets as fd number %d\n\n", i);
					break;
				}
			}
		}
			
		// else IO operation on other socket
		for (int i = 0; i < max_clients; i++){

			int sd = client_socket[i];
			if (FD_ISSET(sd, &readfds)){ // checking which fd has a pending request

				// if client is closing valread = 0 else > 0
				if ((valread = read(sd, commandName, dataSize)) == 0){
					// print disconnected client details
					getpeername(sd, (struct sockaddr*)&address, \
								(socklen_t*)&addrlen);

					for(int j=0; j<currTopicCount; j++) // unsubscribing client sd from all topics
						clientSubscribe[j][sd] = false; // same sd can be used by other clients
					printf("[+]Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					//Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;
				}
					
				// if msg is sent to server
				else{
					// socket desccriptor of requesting client
					getpeername(sd, (struct sockaddr*)&address, \
								(socklen_t*)&addrlen);
					sprintf(rqstPortNo, "%d", ntohs(address.sin_port)); // port number

					if(!strcmp(commandName, "list")){ // to request topics listing
						printf("[:]Client %s requests server to list topic names.\n", rqstPortNo);
						send(sd, "list", dataSize, 0);
						send_list(sd); // send topic list
					}
					else if(!strcmp(commandName, "subscribe")){ // to subscribe to topic
						read(sd, clientTopic, dataSize);
						printf("[:]Client %s wants to subscribe to topic - %s\n", rqstPortNo, clientTopic);
						
						bool flag = false;
						for(int j=0; j<currTopicCount; j++){ // check whether topic is available
							if(!strcmp(topics[j], clientTopic)){
								clientSubscribe[j][i] = true; // if available then subscribe
								printf("[+]Topic Available. Client Subscribed.\n");
								flag = true;
							}
						}
						if(!flag){ // else send error msg to client
							strcpy(stdErr, "Requested topic is unavailable - ");
							strcat(stdErr, clientTopic);
							send(sd, "ERROR", dataSize, 0); // sending error message to rqsting port
							send(sd, stdErr, dataSize, 0);
							printf("[-]Topic Unavailable. Informed Client.\n");
						}
					}else if(!strcmp(commandName, "news")){ // to send a news
						read(sd, clientTopic, dataSize);
						read(sd, clientNews, dataSize);
						printf("[:]Client %s has sent a news for topic -%s\n", rqstPortNo, clientTopic);

						bool flag = false;
						for(int k=0; k<currTopicCount; k++){ // checkking if topic exists
							if(!strcmp(topics[k], clientTopic)){
								flag = true;
								for (int j = 0; j < max_clients; j++){ // sending news to all subscribed clients
									int tempfd = client_socket[j];
									if(clientSubscribe[k][j]){
										send(tempfd, "news", dataSize, 0); 
										send(tempfd, clientTopic, dataSize, 0); 
										send(tempfd, clientNews, dataSize, 0); 
									}
								}
							}
						}
						if(!flag){ // if topic doesnt exist
							strcpy(stdErr, "News sent for the topic is unavailable - ");
							strcat(stdErr, clientTopic);
							send(sd, "ERROR", dataSize, 0); // sending error message to rqsting port
							send(sd, stdErr, dataSize, 0);
							printf("[-]News Topic Unavailable. Informed Client.\n");
						}
					}else{
						printf("[-]Client %s has sent illegal request name. Ignoring.\n", rqstPortNo);
						break;
					}

					printf("[+]Request: %s from port %s successfully executed by the server.\n\n", commandName, rqstPortNo);
					break;
				}
				break;
			}
		}
	}
	close(serverSocket_fd); // closing the server socket
	return 0;
}
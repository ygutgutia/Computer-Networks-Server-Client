// server side code for single server multiple clients
//Compile as : gcc 180001064_server.c -o server
//Execute before clients as : ./server
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
	
#define PORT 11000 // server Port Number is fixed
#define max_clients 10 // can be changed
#define dataSize 256 // size of all buffer set
	
int main(int argc, char *argv[])
{
	int serverSocket_fd, new_socket, client_socket[max_clients], valread;
	struct sockaddr_in address;
	char buffer[dataSize], desnPort[dataSize];
	fd_set readfds; // set of socket descriptors
	char *message = "Server connected. Chat Enabled with other clients.\n"; // broadcasted when new connection is made
	
	// initialise all client_socket[] to 0
	for (int i = 0; i < max_clients; i++)
		client_socket[i] = 0;
	
	// create a server socket
	if( (serverSocket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("socket creaation failed");
		exit(EXIT_FAILURE);
	}
	
	// type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
		
	// bind the socket to la defined port
	if (bind(serverSocket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("binding socket to port number failed.");
		exit(EXIT_FAILURE);
	}
	printf("Listening on port %d \n", PORT);
		
	// maximum 3 pending connections allowed
	if (listen(serverSocket_fd, 3) < 0){
		perror("connection pending rqst overload");
		exit(EXIT_FAILURE);
	}
		
	// accept the incoming connection
	int addrlen = sizeof(address);
	puts("Waiting for connections ...");
		
	while(1){

		FD_ZERO(&readfds); // clear the socket set
		FD_SET(serverSocket_fd, &readfds); // add master socket to set
		int max_sd = serverSocket_fd;
			
		//add child sockets to set
		for (int i = 0 ; i < max_clients ; i++){
			int sd = client_socket[i]; // socket descriptor
			// if socket descriptor is valid then we add to list
			if(sd > 0)
				FD_SET( sd, &readfds);
			// keeps count of highest fd number
			if(sd > max_sd)
				max_sd = sd;
		}
	
		//wait indefinitely for activity on one of the sockets
		int activity = select( max_sd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno!=EINTR))
			printf("error in selecting activity.\n");
			
		// incoming coonnection request
		if (FD_ISSET(serverSocket_fd, &readfds)){

			if ((new_socket = accept(serverSocket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
				perror("Error in socket creation of client");
				exit(EXIT_FAILURE);
			}
			printf("New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
		
			//send new connection greeting message
			if(send(new_socket, message, strlen(message), 0) != strlen(message))
				perror("send");
			puts("Welcome message sent successfully.");
				
			//add new socket to array of sockets
			for (int i = 0; i < max_clients; i++){
				if( client_socket[i] == 0 ){
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n\n", i);
					break;
				}
			}
		}
			
		// else IO operation on other socket
		for (int i = 0; i < max_clients; i++){

			int sd = client_socket[i];
			if (FD_ISSET(sd, &readfds)){

				// if client is closing valread = 0 else > 0
				if ((valread = read(sd, buffer, dataSize)) == 0){
					// print disconnected client details
					getpeername(sd, (struct sockaddr*)&address, \
								(socklen_t*)&addrlen);
					printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					//Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;
				}
					
				// if msg is sent to server
				else{

					printf("Data read - \"%s\" - Now waiting for Destination Port Number\n", buffer); // stored in buffer
					read(sd, desnPort, dataSize); // reading destination port number
					int desnIntport = atoi(desnPort);
					printf("Client says- \"%s\" - and wants to send data to port %d\n", buffer, desnIntport);

					int flag=0; // checks whether desnPort exists
					for(int j=0; j<max_clients; j++){
						getpeername(client_socket[j], (struct sockaddr*)&address, \
									(socklen_t*)&addrlen);
						if(ntohs(address.sin_port) == desnIntport){ // if current fd port number if desnPort
							printf("Destination Port Found\n");
							flag++;
							send(client_socket[j], buffer, strlen(buffer), 0); // send data to destination
							printf("Data Sent\n\n");
							break;
						}
					}
					if(flag==0) // if desnPort is not found
						printf("Destination Port %d not found.\n\n", desnIntport);
					
				}
				memset(buffer, 0, sizeof(buffer)); // resetting the buffers
				memset(desnPort, 0, sizeof(desnPort));
				break;
			}
		}
	}
		
	close(serverSocket_fd);
	return 0;
}
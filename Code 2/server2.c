// server side code for single server multiple clients
// compile as : gcc 180001064_server.c -o server
// execute before clients as : ./server
// acts as an intermediary for msg or file passing
// change the IP address used in both codes to your system's IP addresss
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
#include <sys/stat.h> // for checking if dir exists
#include <dirent.h> // to access directories
	
#define PORT 10000 // server Port number is fixed
#define max_clients 10 // can be changed
#define dataSize 256 // size of all buffer set
#define fileDataSize 1024 // each file's max data size in one line

// function to recieve and write file
void write_file(int sockfd, char *fileNameBuffer){
	FILE *fp;
	char buffer[fileDataSize];
	fp = fopen(fileNameBuffer, "w"); // opening/creating file
	while(1){ // keep reading until it reaches end of file
		read(sockfd, buffer, fileDataSize); // read data from socket
		if(!strcmp(buffer, "END")) // if server sends END, it indicates end of file
			break;
		fprintf(fp,"%s", buffer); // store data in file
		bzero(buffer, fileDataSize); // clear buffer
	}
	fclose(fp); // closing file descriptor
	return;
}

// function to send file data to server
int send_file(FILE *fp, int sockfd){
	char data[fileDataSize] = {0};
	while(fgets(data, fileDataSize, fp) != NULL){ // read file data line by line
		if (send(sockfd, data, fileDataSize, 0) == -1){ // error in sending data
			send(sockfd, "END", fileDataSize, 0); // mark end of sending
			return 0; // return 0 incase of an error
		}
		bzero(data, fileDataSize); // clear buffer
	}
	send(sockfd, "END", fileDataSize, 0); // mark end of file
	return 1;
}	

int main(int argc, char *argv[])
{
	int serverSocket_fd, new_socket, client_socket[max_clients], valread;
	struct sockaddr_in address;
	// buffers to store client messages
	char commandName[dataSize], desnPort[dataSize], fileNameBuffer[dataSize], rqstPortNo[dataSize], stdErr[dataSize];
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
		int max_sd = serverSocket_fd;

		memset(commandName, 0, dataSize); // resetting the buffers
		memset(desnPort, 0, dataSize);
		memset(fileNameBuffer, 0, dataSize);
		memset(rqstPortNo, 0, dataSize);
		memset(stdErr, 0, dataSize);
			
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
			printf("[-]error in selecting activity.\n");
			
		// incoming coonnection request
		if (FD_ISSET(serverSocket_fd, &readfds)){

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
					read(sd, desnPort, dataSize); // reading destination port number
					int desnIntport = atoi(desnPort);
					if(!strcmp(commandName, "list")) // to request file listing
						printf("[:]Client %s requests %s to list file names.\n", rqstPortNo, desnPort);
					else if(!strcmp(commandName, "send")){ // to request sending file
						read(sd, fileNameBuffer, dataSize);
						printf("[:]Client %s requests client %s to send file name \"%s\"\n", rqstPortNo, desnPort, fileNameBuffer);
					}else if(!strcmp(commandName, "message")){ // to send a message
						read(sd, fileNameBuffer, dataSize);
						printf("[:]Client %s wants to send message client %s - \"%s\"\n", rqstPortNo, desnPort, fileNameBuffer);
					}else if(!strcmp(commandName, "listing files")) // to send listed file names
						printf("[:]Client %s is listing files to client %s.\n", rqstPortNo, desnPort);
					else if(!strcmp(commandName, "sending file")){ // to send entire file
						read(sd, fileNameBuffer, dataSize);
						printf("[:]Client %s is sending file %s to port %s\n", rqstPortNo, fileNameBuffer, desnPort);
						write_file(sd, fileNameBuffer);
						printf("[+]File recieved by server.\n");
					}
					else if(!strcmp(commandName, "ERROR")){
						read(sd, fileNameBuffer, dataSize);
						printf("[-]Client %s has error:\"%s\" for the port %s\n", rqstPortNo, fileNameBuffer, desnPort);
					}else{
						printf("[-]Client %s has sent illegal request name. Ignoring.\n", rqstPortNo);
						break;
					}

					int flag=0;
					for(int j=0; j<max_clients; j++){ // checks whether desnPort exists
						getpeername(client_socket[j], (struct sockaddr*)&address, \
									(socklen_t*)&addrlen);
						if(ntohs(address.sin_port) == desnIntport){ // if current fd port number is desnPort
							printf("[+]Destination Port Found\n");
							flag++;
							send(client_socket[j], commandName, dataSize, 0); // send command name to destination
							send(client_socket[j], rqstPortNo, dataSize, 0); // send requestig port name to destination
							if(!strcmp(commandName, "send") || !strcmp(commandName, "ERROR") || !strcmp(commandName, "message"))
								send(client_socket[j], fileNameBuffer, dataSize, 0); // just end the data
							else if(!strcmp(commandName, "listing files")){
								while(1){
									memset(fileNameBuffer, 0, dataSize);
									read(sd, fileNameBuffer, dataSize); // recieve all file names
									if(!strcmp(fileNameBuffer, "END"))
										break;
									send(client_socket[j], fileNameBuffer, dataSize, 0); // send filenames to desn socket
								}
								send(client_socket[j], "END", dataSize, 0); // to denote end of filenames
							}else if(!strcmp(commandName, "sending file")){
								printf("[:]Sending recieved file data to port %s\n", desnPort);
								send(client_socket[j], fileNameBuffer, dataSize, 0);
								int flag = 1;
								FILE *fp;
								fp = fopen(fileNameBuffer, "r"); // open file
								if(fp == NULL){ 
									printf("[-]Error in reading the file. File sent to be failed.\n");
									flag = 0;
									remove(fileNameBuffer); // deleting file from the server
									break;
								}
								if(send_file(fp, client_socket[j]))
									printf("[+]File sent successfully to destination.\n");
								else{
									printf("[-]Data Corrupted while sending.\n");
									flag = 0;
								}
								remove(fileNameBuffer);
								if(flag == 0){
									strcpy(stdErr, "File corrupted while sending from server. Request again.");
									send(client_socket[j], "ERROR", dataSize, 0); // sending error message to rqsting port
									char strServerPortNo[dataSize];
									sprintf(strServerPortNo, "%d", PORT);
									send(client_socket[j], strServerPortNo, dataSize, 0);
									send(client_socket[j], stdErr, dataSize, 0);
								}
							}
							printf("[+]Request: %s from port %s to port %s successfully executed by server.\n\n", commandName, rqstPortNo, desnPort);
							break;
						}
					}
					if(flag==0){ // if desnPort is not found
						printf("[-]Destination Port %s not found. Letting requesting port know this.\n\n", desnPort);
						strcpy(stdErr, "Not found Port ");
						strcat(stdErr, desnPort);
						send(sd, "ERROR", dataSize, 0); // sending error message to rqsting port
						char strServerPortNo[dataSize];
						sprintf(strServerPortNo, "%d", PORT);
						send(sd, strServerPortNo, dataSize, 0);
						send(sd, stdErr, dataSize, 0);
					}
					
				}
				break;
			}
		}
	}
	close(serverSocket_fd); // closing the server socket
	return 0;
}
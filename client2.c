//Client side code for single server multiple clients
//Clients can pass messages or transfer files.
//Five commands are defined: send, list, close, message, clear
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
#include <sys/stat.h> // for checking if dir exists
#include <dirent.h> // to access directories

#define SERV_PORT 10000 //server port - same as in server code
#define STDIN 0 // stdin fd is 0
#define dataSize 256 // size of all buffer set
#define maxFileCount 50 // can change to store more number of files
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
	// buffer are for message communication
	char userCmnd[dataSize], userFileName[dataSize], userPortNo[dataSize], stdErr[dataSize],
		serverCmnd[dataSize], serverfileName[dataSize], serverPortNo[dataSize], clientPortNo[dataSize];
	struct sockaddr_in server_addr, client_addr;
	//set of socket descriptors
	fd_set readfds;
	int currFileCount = 0; // number of files with the socket
	char clientDirPath[] = {'.', '/', '\0'}; // stores client folder directory
	char filesList[maxFileCount][dataSize]; // max 50 files possible

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
	server_addr.sin_addr.s_addr = inet_addr("10.0.2.15"); // enter system's machine IP
	server_addr.sin_port = htons(SERV_PORT);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_addr.s_addr = inet_addr("10.0.2.15"); // enter system's machine IP
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
	strcat(clientDirPath, clientPortNo); // client directory has the same name as it's port number
	struct stat st = {0};
	if (stat(clientDirPath, &st) == -1) // checks if dir already present
		mkdir(clientDirPath, 0700); // create a new dir if not present
	// opendir() returns a pointer of DIR type. 
    DIR *dir = opendir(clientDirPath);
    struct dirent *fileName; // pointer for directory files
    while ((fileName = readdir(dir)) != NULL){ // iterating through all the files
        if (!strcmp(fileName->d_name, ".") || !strcmp(fileName->d_name, "..")){ // skip
        }else{
			// store filename and increase count
			strcpy(filesList[currFileCount], fileName->d_name);
			currFileCount++;
        }
    }
    closedir(dir);

	
	while(1){

		memset(userCmnd, 0, dataSize); // resetting all buffers
		memset(userPortNo, 0, dataSize);
		memset(userFileName, 0, dataSize);	
		memset(serverCmnd, 0, dataSize);	
		memset(serverPortNo, 0, dataSize);
		memset(serverfileName, 0, dataSize);
		memset(stdErr, 0, dataSize);

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
			if(!strcmp(userCmnd, "list")){
			}else if(!strcmp(userCmnd, "send")){
				printf("[..]Enter File name\n");
				fgets(userFileName, dataSize, stdin); // send command requires name of file
				userFileName[strcspn(userFileName, "\n")] = 0; //removing \n from end of line
			}else if(!strcmp(userCmnd, "close")){
				printf("[+]CLosing server....\n");
				//close(clientfd);
				break;
			}else if(!strcmp(userCmnd, "message")){
				printf("[..]Enter Message\n");
				fgets(userFileName, dataSize, stdin); // message to be sent must be entered
				userFileName[strcspn(userFileName, "\n")] = 0; //removing \n from end of line
			}else if(!strcmp(userCmnd, "clear")){
				system("clear");
				continue;
			}else{
				printf("[-]FTP Commands allowed:\na. list\nb. send\nc. close\nd. message\ne. clear\n");
				continue;
			}
			
			printf("[..]Enter Destination port number.\n");
			fgets(userPortNo, dataSize, stdin); // port number of the destination client
			userPortNo[strcspn(userPortNo, "\n")] = 0;
			send(clientfd, userCmnd, dataSize, 0); // sending data to server
			send(clientfd, userPortNo, dataSize, 0);

			if(!strcmp(userCmnd, "send") || !strcmp(userCmnd, "message"))
				send(clientfd, userFileName, dataSize, 0); // send additional data
        }

        else if (FD_ISSET(clientfd, &readfds)){ // if message recieved from server
			read(clientfd, serverCmnd, dataSize); // read command name
			read(clientfd, serverPortNo, dataSize); // read requesting port
			if(!strcmp(serverCmnd, "list")){
				printf("[:]Server sent request: list files to port number %s\n", serverPortNo);
				send(clientfd, "listing files", dataSize, 0); // send command to server
				send(clientfd, serverPortNo, dataSize, 0); // send port number of the port which rqsted list
				for(int i=0; i<currFileCount; i++)
					send(clientfd, filesList[i], dataSize, 0); // send filenames
				send(clientfd, "END", dataSize, 0); // to denote end
				printf("[+]File names sent back to requesting port.\n");
			}else if(!strcmp(serverCmnd, "send")){
				read(clientfd, serverfileName, dataSize); // read filename
				printf("[+]Server sent request: send %s to port number %s\n", serverfileName, serverPortNo);
				int flag = 0;
				for(int i=0; i<currFileCount; i++){
					if(!strcmp(filesList[i], serverfileName)){
						flag = 1;
						printf("[+]File present. Preparing to send.\n");
						send(clientfd, "sending file", dataSize, 0); // send command to server
						send(clientfd, serverPortNo, dataSize, 0); // send port number of the port which rqsted list
						send(clientfd, serverfileName, dataSize, 0); // send fileName
						char pathFile[] = {'.', '/', '\0'}; // stores relative path of the file
						strcat(pathFile, clientPortNo);
						strcat(pathFile, "/");
						strcat(pathFile, serverfileName);
						FILE *fp;
						fp = fopen(pathFile, "r"); // open file
						if (fp == NULL){
							flag = 0;
							break;
						}
						if(send_file(fp, clientfd))
							printf("[+]File data sent successfully.\n");
						else{
							printf("[-]Data Corrupted while sending\n");
							flag = 0;
						}
						break;
					}
				}
				if(flag == 0){
					printf("[-]Error reading File Name %s.\n", serverfileName);
					printf("[:]File is not available or corrupted while sending the server. Letting requesting port know this.\n\n");
					strcpy(stdErr, "Not found filename ");
					strcat(stdErr, serverfileName);
					send(clientfd, "ERROR", dataSize, 0);
					send(clientfd, serverPortNo, dataSize, 0);
					send(clientfd, stdErr, dataSize, 0);
				}
			}else if(!strcmp(serverCmnd, "listing files")){
				printf("[+]Port %s has sent a list of files they have:\n", serverPortNo);
				while(1){
					memset(serverfileName, 0, dataSize);
					read(clientfd, serverfileName, dataSize);
					if(!strcmp(serverfileName, "END"))
						break;
					printf("%s\n", serverfileName);
				}
			}else if(!strcmp(serverCmnd, "sending file")){
				read(clientfd, serverfileName, dataSize);
				char clientFilePath[] = {'.', '/', '\0'}; // stores relative path of the file
				strcat(clientFilePath, clientPortNo);
				strcat(clientFilePath, "/");
				strcat(clientFilePath, serverfileName);
				write_file(clientfd, clientFilePath); // recieve data from server and write
				printf("[+]File %s recieved successfully from port %s\n", serverfileName, serverPortNo);
				if(currFileCount == 50) // file will be created but wont be displayed
					printf("[-]File count is 50. Cant add more names to the list.\n");
				else{
					// add file to the list
					strcpy(filesList[currFileCount], serverfileName);
					currFileCount++;
				}
			}else if(!strcmp(serverCmnd, "ERROR")){ // incase of error
				read(clientfd, stdErr, dataSize);
				printf("[-]Server said: %s\n", stdErr);
			}else if(!strcmp(serverCmnd, "message")){
				read(clientfd, serverfileName, dataSize); // reading message
				printf("[+]Message recieved: %s from client %s\n", serverfileName, serverPortNo);
			}
			else
				printf("[-]Invalid command. Ignoring.\n");
        }
	}

	close(clientfd); // close the client
	return 0;
}
## Code 1
In the, we have to write a server side code, and a client side code, such that we can create one server, and multiple clients, and all the clients can communicate with each other via the server.

In the attached codes [server1.c](server1.c) and [client1.c](client1.c), I have used **file descriptors**, and select() to listen to various calls made on a port. The select() tells the server whether a disturbance is caused by a new client trying to connect, or whether it is caused by a message sent by an already connected client, or whether some client has left the disconnected the server.
The server must be compiled and executed before running any clients. The compilation and execution can be done by:
> gcc server1.c -o server
> ./server

This doesn’t require any command line argument. By default, this sets up the port number 11000 as the server port. **Care must be taken to change the IP address of the machine on which the code is being executed manually in both the server and client side code.** After the server is set, the user can create any number of clients. The client execution requires the port number that shall be used, as a command line argument. If the port number used is unavailable, the client binds to a random port number.
> gcc client1.c -o client
> ./client 12010                                            //Client with port number 12010
> ./client 12020                                            //Client with port number 12020
> ./client 12030                                            //Client with port number 12030

After this the user can enter a data in the command line for any client, followed by the port number of another client to which the data must be sent. For example, if the user enters this in the command line of Client 1:
> Hello Peer client
> 12020

The server shall intercept this message and send it to client with port number 12030. Similarly, at any time a client can join or leave the server.
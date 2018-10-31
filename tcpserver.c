#include <sys/socket.h> // program interface to any network socket (generic)
#include <netinet/in.h> // specific to internet protocols (tcp, udp, idp)
#include <arpa/inet.h> // specific to internet protocols (tcp, udp, idp)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int check_port(char*);

// we are having the child run a function
// this function MUST return void* and take void* for arguments of any type
void* handleclient_r(void* arg);

// we are having the child run a function
// this function MUST return void* and take void* for arguments of any type
void* handleclient_w(void* arg);

int main(int argc, char** argv) {
	char port[1024];
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int p;

	// check for error
	if (sockfd < 0) {
		printf("Problem creating socket\n");
		return 1;
	}

	// Check for valid port.
	printf("Please give a port number: \n");
	fgets(port, 1024, stdin);
	port[strcspn(port, "\n")] = 0;
	while(!check_port(port)) {
		printf("Please give a port number: \n");
		fgets(port, 1024, stdin);
		port[strcspn(port, "\n")] = 0;
	}

	p = atoi(port);

	// server needs its own address but should also store client address
	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(p);
	// dont have to program the server to know what device it is running on. 
	// this constant has you use any address available.. 
	// as long as it has that same port number.
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	// opposite of connect. 
	int b = bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	// error check very important here
	if (b < 0) {
		printf("Bind error\n");
		return 3;
	}

	// 10 is the backlog for buffer of connection requests that we havent handled.
	// handles multiple.. but not at the same time...
	listen(sockfd, 10);

	printf("Now listening on the server.\n");

	// we want the server to keep accepting clients. an infinite loop is most common
	while(1) {
		int len = sizeof(clientaddr);
		// accept is a blocking call. it will wait until it can accept something.
		// gives us a new socket back to specifically talk to the client
		int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
		// each client that connects, make a new thread and receive data
		pthread_t child_r, child_w;
		// create and run the thread and start at this handleclient function.
		// pass clientsocket to the child.
		pthread_create(&child_r, NULL, handleclient_r, &clientsocket);
		pthread_create(&child_w, NULL, handleclient_w, &clientsocket);
		// detach because the parent doesn't care about the child.
		// when the thread is done, the OS will just get rid of it.
		pthread_detach(child_r);
		pthread_detach(child_w);
	}

	return 0;
}

/* Check for valid digits in port number. */
int check_port(char* port) {
	int digits = 0;
	for (int i = 0; i < sizeof(port); i++) {
		if ((port[i] != '0') && !atoi(&port[i])) digits++;
	}
	if ((digits > 0 && digits < 6) && (atoi(port) > 0 && atoi(port) < 65536)) return 1;
	else return 0;
}

// we are having the child run a function
// this function MUST return void* and take void* for arguments of any type
void* handleclient_r(void* arg) {
	int clientsocket = *(int*) arg; //integer pointer + dereference to get the integer.
	while(1) {
		char line[5000];
		recv(clientsocket, line, 5000, 0);
		if (strcmp(line, "exit\n") == 0) {
			printf("Shutting down...\n");
			send(clientsocket, line, strlen(line) + 1, 0);
			close(clientsocket);
			exit(0);
		} else {
			printf("Client: %s\n", line);
		}
	}
}

// we are having the child run a function
// this function MUST return void* and take void* for arguments of any type
void* handleclient_w(void* arg) {
	int clientsocket = *(int*) arg; //integer pointer + dereference to get the integer.
	while(1) {
		char line[5000];
		fgets(line, 5000, stdin);
		if (strcmp(line, "exit\n") == 0) {
			printf("Shutting down...\n");
			send(clientsocket, line, strlen(line) + 1, 0);
			close(clientsocket);
			exit(0);
		} else {
			send(clientsocket, line, strlen(line) + 1, 0);
		}
	}
}
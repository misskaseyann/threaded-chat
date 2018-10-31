#include <sys/socket.h> // program interface to any network socket (generic)
#include <netinet/in.h> // specific to internet protocols (tcp, udp, idp)
#include <arpa/inet.h> // specific to internet protocols (tcp, udp, idp)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define DELIM "."

int check_port(char*);

// we are having the child run a function
// this function MUST return void* and take void* for arguments of any type
void* handleserver(void* arg);

// making a client and server
int main(int argc, char** argv) {
	char addr[1024], port[1024];
	unsigned long ip = 0;
	int p;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		printf("There was an error creating the socket\n");
		return 1;
	}

	// Check to make sure the address is valid, exit otherwise.
	printf("Please give an address: \n");
	fgets(addr, 1024, stdin);
	addr[strcspn(addr, "\n")] = 0;
	if (0 == inet_pton(AF_INET, addr, &ip)) {
        printf("Failed\n");
        exit(0);
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
	// set up the specific place the socket goes to.. ->socket address internet
	struct sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET; // family of INET
	// what application we should hand a packet to.. the server is sending data to this port. 
	// h to ns, different representations of integers. 
	// always use htons() so you never have to worry about remembering. htons means: host to network short
	serveraddr.sin_port=htons(p); 
	// address is acting like you are sending over the network but *really* you are sending back to yourself.
	// useful to use when you don't want to be specifying your IP
	serveraddr.sin_addr.s_addr=inet_addr(addr);

	// casting down to generic type sockaddr since we are not looking for the internet socket address
	int e = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	// check for errors
	if (e < 0) {
		printf("There was an error connecting\n");
		return 2;
	}

	printf("Now connected...\n");

	// at this point our socket is connected to a remote server and we can start communicating

	pthread_t child;
	pthread_create(&child, NULL, handleserver, &sockfd);
	pthread_detach(child);

	while(1) {
		char line[5000];
		fgets(line, 5000, stdin);
		if (strcmp(line, "exit\n") == 0) {
			printf("Shutting down...\n");
			send(sockfd, line, strlen(line) + 1, 0);
			close(sockfd);
			exit(0);
		} else {
			send(sockfd, line, strlen(line) + 1, 0);
		}
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
void* handleserver(void* arg) {
	int clientsocket = *(int*) arg; //integer pointer + dereference to get the integer.
	while(1) {
		char line[5000];
		// uses client socket to talk to that client.
		// client socket allows us to distinguish between clients.
		recv(clientsocket, line, 5000, 0);
		if (strcmp(line, "exit\n") == 0) {
			printf("Shutting down...\n");
			send(clientsocket, line, strlen(line) + 1, 0);
			close(clientsocket);
			exit(0);
		} else {
			printf("Server: %s\n", line);
		}
	}
}

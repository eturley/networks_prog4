//Marco Bruscia, Erin Turley, Michael Parowski
//netid: jbruscia, eturly, mparowsk
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <math.h>
#define MAX_LINE 4096
#define CHUNK_SIZE 1024
#define MAX_PENDING 5

int fsize(const char *);
double sec_usec(int, int);

int fsize(const char *filename) {
	struct stat st;

	if(stat(filename, &st) == 0)
		return st.st_size;
	return -1;
}

double sec_usec(int a, int b) {
	int decimals = log10(b) + 1;
	return a + b*pow(10.0, -decimals);
}

int main(int argc, char * argv[]) {


	char timeStamp[90];
	time_t ltime;
	struct tm *Tm;
	struct timeval tv;
	int stampLength, i;
	struct sockaddr_in sin, client_addr;
	char buf[MAX_LINE];
	int len, addr_len,s, new_s, f, status, fd;
	int portNum, messageLength, bytes_sent;
	char *fullMessage;
	char encMessage[4096];
	char *encKey;
	char *opt;
	DIR *d;
	struct dirent *de;
	struct stat dirbuf;
	int exists, bytes_read, bytes_written, bytes;
	int total_size;
	char path[256], perms[MAX_LINE];

	if (argc == 2){
		portNum = atoi(argv[1]);
	}
	else {
		printf("Incorrect number of arguments. Usage: \n ./main [port number] [encryptionkey] \n");
		exit(1);
	}

	/* build address data structure */ 
	bzero ((char *)&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	sin.sin_addr.s_addr = INADDR_ANY; //Use the default IP address of server 
	sin.sin_port = htons(portNum); 


	/* setup passive open*/
	if ((s = socket(PF_INET, SOCK_STREAM,0)) < 0) {
		perror("simplex-talk:socket");
	}

	/* set socket option*/
	if((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)& opt, sizeof(int))) < 0) {
		perror("simplex-talk:setsockopt\n");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0) {
		perror("simplex-talk: bind");
		exit(1);
	}

	addr_len = sizeof(sin);

	if((listen(s, MAX_PENDING)) < 0) {
		perror("simplex-talk: bind\n");
	}
	while (1) {
		if((new_s = accept(s, (struct sockaddr *)&sin, &addr_len)) < 0) {
			perror("simplex-talk: accept\n");
			exit(1);
		}
	}
}

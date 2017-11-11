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
#include <pthread.h>
#define MAX_LINE 4096
#define CHUNK_SIZE 1024
#define MAX_PENDING 5

void *connection_handler(void *socket_desc);

int main(int argc, char * argv[]) {
  char timeStamp[90];
  time_t ltime;
  struct tm *Tm; 
  struct timeval tv;
  int stampLength, i, len, addr_len,s, new_s, f, status, fd, portNum, messageLength, bytes_sent;
  struct sockaddr_in sin, client_addr;
  //struct sockaddr_in client_addr;
  char buf[MAX_LINE], *fullMessage, encMessage[4096], *encKey, *opt;
  DIR *d;
  struct dirent *de;
  struct stat dirbuf;
  int exists, bytes_read, bytes_written, bytes, total_size;
  char path[256], perms[MAX_LINE];

  //proj4 variables
  int client_sock, NUM_THREADS = 0;
  
  if (argc == 2){
    portNum = atoi(argv[1]);
  }
  else {
    printf("Incorrect number of arguments. Usage: \n ./main [port number] \n");
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
  printf("Waiting for incoming connections...");
  pthread_t thread;
    
  while((client_sock = accept(s, (struct sockaddr *)&sin, &addr_len)) > 0) {
    if(NUM_THREADS == 10){
      printf("Connection Refused: Max Clients Online.\n");
      continue;
    }
    printf("Connection accepted.\n");
    NUM_THREADS++;
    if(pthread_create(&thread, NULL, connection_handler, (void*) &client_sock) < 0) {
      perror("error creating thread");
      return 1;
    }
    //pthread_join(thread, NULL);
  }
  if(close(s) != 0){
    printf("Socket was not closed\n");
  }
}


//handles each client connection
void *connection_handler(void *socket_desc) {
  printf("inside connection handler!\n");
  int sock = *(int*)socket_desc;
  char *msg, client_msg[1024];
  //LOGIN
  if(recv(sock, client_msg, sizeof(client_msg), 0) == -1){
    perror("receive error\n");
    exit(1);
  }
  printf("username: %s\n", client_msg);
  


  //Receive username from client and check status (new or existing user)
  
  //Server requests password

  //Receive password from client

  //Either register new user or check if password matches

  //Send acknowledgement to client

  //wait for operation command (while loop)
}

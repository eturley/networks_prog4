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
  int sock = *(int*)socket_desc;
  char server_msg[1024], client_msg[1024], username[1024], password[1024];
  char * line = NULL;
  int returning = 0, valid_login = 0;
  size_t len = 0;
  ssize_t read;
  //receive username from client
  if(recv(sock, client_msg, sizeof(client_msg), 0) == -1){
    perror("receive error\n");
    exit(1);
  }
  //check if new or existing user
  strcpy(username, client_msg);
  FILE *fp = fopen("login.txt", "ab+");
  while((read = getline(&line, &len, fp)) != -1){
    char *token = strtok(line, ":");
    if(strcmp(token, client_msg) == 0){
      returning = 1;
    }
  }
  if(!returning){
    //fprintf(fp, client_msg);
    strcpy(server_msg, "Welcome, new user! Please enter password:\n");
  }
  else {
    strcpy(server_msg, "Welcome, returning user! Please enter password:\n");
  }
  fclose(fp);
  //Server requests password
  if(send(sock, server_msg, sizeof(server_msg) + 1, 0) == -1){
    perror("Server send error\n");
    exit(1);
  }

  //Receive password from client
  bzero((char *)& client_msg, sizeof(client_msg));
  if(recv(sock, client_msg, sizeof(client_msg) + 1, 0) == -1){
    perror("Server receive error\n");
    exit(1);
  }
  strcpy(password, client_msg);
  printf("password: %s", password);

  //Either register new user or check if password matches
  FILE *fp2 = fopen("login.txt", "ab+");
  if(returning){
    while((read = getline(&line, &len, fp2)) != -1){
      char *token = strtok(line, ":");
      char *tok2 = strtok(NULL, ":");
      if((strcmp(username, token) == 0) && (strcmp(password, tok2) == 0)){
	printf("it's a match!\n");
	valid_login = 1;
      }
    }
  }
  else {
    strcat(username, ":");
    fprintf(fp2, username);
    strcat(password, "\n");
    fprintf(fp2, password);
    printf("inserted user!\n");
    valid_login = 1;
  }
  fclose(fp2);

  //Send acknowledgement to client
  /*bzero((char *)& server_msg, sizeof(server__msg));
  if(valid_login){
    strcpy(server_msg, "You are now logged in!\n");
  } else {
    strcpy(server_msg, "Incorrect password.\n");
    }*/
  //might have to do htons stuff?
  if(send(sock, &valid_login, sizeof(valid_login), 0) == -1){
    perror("Server send error\n");
    exit(1);
  }  

  //wait for operation command (while loop)
}

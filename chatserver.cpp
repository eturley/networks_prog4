//Marco Bruscia, Erin Turley, Michael Parowski
//netid: jbruscia, eturley, mparowsk
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
#include <iostream>
#include <string>
#include <vector>
#define MAX_LINE 4096
#define CHUNK_SIZE 1024
#define MAX_PENDING 5

using namespace std;

void *connection_handler(void *socket_desc);
vector<pair<string, int> > current_users;

int main(int argc, char * argv[]) {
  char timeStamp[90];
  time_t ltime;
  struct tm *Tm; 
  struct timeval tv;
  int stampLength, i, len, s, new_s, f, status, fd, portNum, messageLength, bytes_sent;
  struct sockaddr_in sin, client_addr;
  char buf[MAX_LINE], *fullMessage, encMessage[4096], *encKey, *opt;
  DIR *d;
  struct dirent *de;
  struct stat dirbuf;
  int exists, bytes_read, bytes_written, bytes, total_size;
  char path[256], perms[MAX_LINE];

  //proj4 variables
  int  NUM_THREADS = 0;
  int client_sock;
  socklen_t addr_len;

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
    NUM_THREADS++;//mutex lock
    if(pthread_create(&thread, NULL, connection_handler, (void*) &client_sock) < 0) {
      perror("error creating thread");
      return 1;
    }
    //pthread_join(thread, NULL);
  }
  if(close(s) != 0){
    printf("Socket was not closed\n");
  }
  else
    printf("Connection closed\n");
}


//handles each client connection
void *connection_handler(void *socket_desc) {
  int leave = 0;
  int sock = *(int*)socket_desc;
  char server_msg[1024], client_msg[1024], username[1024], password[1024];
  char * line = NULL;
  int returning = 0, valid_login = 0;
  size_t len = 0;
  ssize_t read;
    
  if(recv(sock, username, sizeof(username)+1, 0) == -1){
    perror("receive error\n");
    //exit(1);
  }
  
  FILE *fp = fopen("login.txt", "ab+"); //lock
  while((read = getline(&line, &len, fp)) != -1){
    char *token = strtok(line, ":");
    if(strcmp(token, username) == 0){
      returning = 1;
    }
  }
  if(!returning){
    strcpy(server_msg, "Welcome, new user! Please enter password:\n");
  }
  else {
    strcpy(server_msg, "Welcome, returning user! Please enter password:\n");
  }
  fclose(fp);
  //Server requests password
  if(send(sock, server_msg, sizeof(server_msg) + 1, 0) == -1){
    perror("Server send error\n");
  }

  //Receive password from client
  bzero((char *)& client_msg, sizeof(client_msg));
  if(recv(sock, client_msg, sizeof(client_msg) + 1, 0) == -1){
    perror("Server receive error\n");
    //exit(1);
  }
  strcpy(password, client_msg);
  //Either register new user or check if password matches
  FILE *fp2 = fopen("login.txt", "ab+");
  if(returning){
    while((read = getline(&line, &len, fp2)) != -1){
      char *token = strtok(line, ":");
      char *tok2 = strtok(NULL, ":");
      if((strcmp(username, token) == 0) && (strcmp(password, tok2) == 0)){
      	valid_login = 1;
      }
    }
  }
  else {
    strcat(username, ":");
    fprintf(fp2, username);
    strcat(password, "\n");
    fprintf(fp2, password);
    valid_login = 1;
  }
  fclose(fp2);


  string user;
  //add current users to a vector
  if(valid_login){
    user = username;
    if(user[user.size() - 1] == ':'){
      user = user.substr(0, user.size() - 1);
    }
    current_users.push_back(make_pair(user, sock)); //lock
  }

  if(send(sock, &valid_login, sizeof(valid_login), 0) == -1){
    perror("Server send error\n");
  }  

  //wait for operation
  while (leave == 0) {
    char  a[1024];
    char  b[1024];
    bzero((char *)& a, sizeof(a));
    bzero((char *)& b, sizeof(b));
    char mod[1024];
    strcat(a, "C");
    strcat(b, "D");
    bzero((char *)& client_msg, sizeof(client_msg));
    bzero((char *)& server_msg, sizeof(client_msg));
    bzero((char *)& mod, sizeof(mod));
    if(recv(sock, client_msg, sizeof(client_msg) + 1, 0) == -1){
      perror("Server receive error\n");
    }
    if (client_msg[0] == 'P'){ //private message
      int target_sock, found = 0;
      string private_message;
      //generate list of current users 
      for (auto it = current_users.begin(); it != current_users.end(); it++) {
	if(it->first != user) {
	  strcat(server_msg, (it->first).c_str());
	  strcat(server_msg, "\n");
	}
      }
      //send current users to client 
      strcat(mod, a);
      strcat(mod, server_msg);
      if(send(sock, mod, sizeof(mod) + 1, 0) == -1){
        perror("Server send error\n");
      }
      //receive username of private message 
      bzero((char *)& username, sizeof(client_msg));
      if(recv(sock, username, sizeof(client_msg) + 1, 0) == -1){
        perror("Server receive error\n");
      }
      
      //receive message
      bzero((char *)& client_msg, sizeof(client_msg));
      if(recv(sock, client_msg, sizeof(client_msg) + 1, 0) == -1){
        perror("Server receive error\n");
      }
      
      //find socket number for target user
      for (auto it = current_users.begin(); it != current_users.end(); it++) {
        if (strcmp((it->first).c_str(), username) == 0) {
          target_sock = it->second;
          found = 1;
          break;
        }
      }
      
      //send to target user
      bzero((char *)& mod, sizeof(mod));
      strcat(mod, b);
      strcat(mod, client_msg);
      if (found == 1) {
        if(send(target_sock, mod, sizeof(mod) + 1, 0) == -1){
          perror("Server send error\n");
        }
      }
      
      //send confirmation or that user did not exist
      bzero((char *)& server_msg, sizeof(client_msg));
      if (found == 1) {
        strcpy(server_msg, "Message Sent: Confirmed\n");
        bzero((char *)& mod, sizeof(mod));
        strcat(mod, a);
        strcat(mod, server_msg);
        if(send(sock,mod, sizeof(mod) + 1, 0) == -1){
          perror("Server send error\n");
        }
      } else {
        strcpy(server_msg, "User did not exist.\n");
        bzero((char *)& mod, sizeof(mod));
        strcat(mod,a);
        strcat(mod, server_msg);
        if(send(sock, mod, sizeof(mod) + 1, 0) == -1){
          perror("Server send error\n");
        }
      }
    }
      
    else if(client_msg[0] == 'B') { //broadcast message
      // send ack to client to prompt for message to be sent
      bzero((char *)& server_msg, sizeof(server_msg));
      strcpy(server_msg, "Broadcast command received. Ready to receive message\n");
      bzero((char *)& mod, sizeof(mod));
      strcat(mod, a);
      strcat(mod, server_msg);
      if(send(sock, mod, sizeof(mod) + 1, 0) == -1){
        perror("Server send error\n");
      }

      // receive message from client
      bzero((char *)& client_msg, sizeof(client_msg));
      if(recv(sock, client_msg, sizeof(client_msg) + 1, 0) == -1){
        perror("Server receive error\n");
      }

      // send message to all clients
      bzero((char *)& mod, sizeof(mod));
      strcat(mod, b);
      strcat(mod, client_msg);
      for (auto it = current_users.begin(); it != current_users.end(); it++) {
	if(it->first != user) {
	  if(send(it->second, mod, sizeof(mod) + 1, 0) == -1) {
	    perror("Server send broadcast message error");
	    exit(1);
	  }
	}
      }

      // send confirmation that message was sent
      bzero((char *)& mod, sizeof(mod));
      bzero((char *)& server_msg, sizeof(client_msg));
      strcat(mod, a);
      strcpy(server_msg, "Broadcast message was successfully sent.\n");
      strcat(mod, server_msg);
      if(send(sock, mod, sizeof(mod) + 1, 0) == -1)
	perror("Server confirmation send error");
      
    }
    
    else if(client_msg[0] == 'E'){ //exiting
      //remove username from current users
      int index, i = 0;
      for (auto it = current_users.begin(); it != current_users.end(); it++) {
        if (strcmp((it->first).c_str(), user.c_str()) == 0) {
	  index = i;
	}
	i++;
      }
      char exit_signal[1024];
      strcat(exit_signal, "exit");
      current_users.erase(current_users.begin() + index);
      if(send(sock, exit_signal, sizeof(exit_signal) + 1, 0) == -1){
	perror("Server send error\n");
      }
      if (close(sock) != 0) {
	perror("Was not closed!\n");
      }
      break;
    } 
  }

  //begin shutdown protocol
}

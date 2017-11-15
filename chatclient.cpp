//Marco Bruscia, Erin Turley, Mike Parowski
//netid: jbruscia, eturley, mparowsk

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <vector>
#define MAX_LINE 4096
using namespace std;

void *handle_messages(void *socket_desc);
void private_message();
void broadcast();
vector<string> command_messages;

int main(int argc, char * argv[]) {
    
    //initialize vaiables
    FILE *fp;
    long lSize;
    struct hostent *hp;
    struct sockaddr_in sin;
    socklen_t addr_len;
    char buf[MAX_LINE];
    int s, len, ServerPort, fd, bytes_read, bytes_written, exit == 0;

    //proj4 variables
    char *username;
    char *host;
    char *op;
    
    //Command Line Arguments
    if (argc == 4) {
        host = argv[1];
        ServerPort = atoi(argv[2]);
	username = argv[3];
    } else {
        printf("Incorrect amount of arguments. Usage: \n ./main [host name] [port number] [username] \n" );
        exit(1);
    }

    /* translate host name into peer's IP address */
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }
    
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(ServerPort);
    addr_len = sizeof(sin);
    
    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error!");
        exit(1);
    }

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      perror("simplex-talk: connect\n");
      close(s);
      exit(1);
    }
    
    //LOGIN
    //client sends username to server
    if(send(s, username, strlen(username) + 1, 0) == -1){
      perror("client send error\n");
      exit(1);
    }
    bzero((char *)& buf, sizeof(buf));
    if(recv(s, buf, sizeof(buf) + 1, 0) == -1){
      perror("Client receive error\n");
      exit(1);
    }
    else{
      printf("%s", buf);
    }

    //prompt for password
    bzero((char *)& buf, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    if(send(s, buf, sizeof(buf) + 1, 0) == -1) {
      perror("client send error\n");
      exit(1);
    }

    int confirm;
    if(recv(s, &confirm, sizeof(confirm), 0) == -1){
      perror("Client receive error\n");
      exit(1);
    }
    if(confirm){
      printf("You are now logged in!\n");
    } else {
      printf("Incorrect password.\n");
      exit(1);
    }
    
    //create thread that handles messages
    pthread_t thread;
    int rc = pthread_create(&thread, NULL, handle_messages, NULL);
    
    if(rc){
      printf("Error: unable to create thread\n");
      exit(-1);
    }
    while(exit == 0) {
      string curr_message;
      printf("Welcome to Online Chat Room!\n\tEnter P for private conversation.\n\tEnter B for message broadcasting.\n\tEnter E to exit.\n");

        printf(">> ");
        fgets(op, sizeof(op), stdin);
        if(strcmp(op, "P\n")==0){
          if(send(s, op, strlen(op) + 1, 0) == -1){
            perror("client send error\n");
            //exit(1);
           }
        	//lock
        	while(1) { //get list of current users
        	  if(command_messages.size() > 0) {
        	    curr_message = command_messages[0];
        	    command_messages.erase(0);
        	    break;
      	    }
        	}
        	
        	//print out current users 
        	
        	//prompt user for username
        	
        	//send username
        	
        	//prompt for message to be sent
        	
        	//send message
        	
        	//receive confirmation from server_msg
        	//lock
        	while(1) { //get list of current users
        	  if(command_messages.size() > 0) {
        	    curr_message = command_messages[0];
        	    command_messages.erase(0);
        	    break;
      	    }
        	}
        	//print out confirmation
        	
        	private_message();
        } else if(strcmp(op, "B\n") == 0){
        	broadcast();
        } else if(strcmp(op, "E\n") == 0){
        	break;
	  //exit(1);
        } else {
	        printf("Invalid Entry\n");
        }
      
      if(close(s) != 0) {
        printf("Socket was not closed\n");
      }
    }
}

//determine if data or command message
void *handle_messages(void *socket_desc){
  string message;
  string user = "temp user";
  while(exit == 0) {
    bzero((char *)& buf, sizeof(buf));
      if(recv(s, buf, sizeof(buf) + 1, 0) == -1){
        perror("Client receive error\n");
        //exit(1);
    }
    message = buf;
    //send command messages to command vector and print messages from other clients
    if(message[0] == "C") {
      command_messages.push_back(message) //lock
    } else {
      message.erase(0,1);
      cout << "#### New Message: Message Received from " << user << ": " << message << "####\n" << endl 
    }
  }
}

void private_message(){
  //return;
}

void broadcast(){
  //return;
}

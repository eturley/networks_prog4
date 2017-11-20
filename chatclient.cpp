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
void prompt_for_input();
vector<string> command_messages;
char buf[MAX_LINE];
int s, leave = 0;

int main(int argc, char * argv[]) {
    
    //initialize vaiables
    FILE *fp;
    long lSize;
    struct hostent *hp;
    struct sockaddr_in sin;
    socklen_t addr_len;
    //char buf[MAX_LINE];
    int len, ServerPort, fd, bytes_read, bytes_written;
    //int s;
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
    while(leave == 0) {
      cout << "in while loop" << endl;
      string curr_message; 
      string message_to_send; 
      string target_user; 
      string message_type;
      prompt_for_input();
      getline(cin, message_type);

      //Private Message
      if(strcmp(message_type.c_str(), "P")==0){
	if(send(s, message_type.c_str(), strlen(message_type.c_str()) + 1, 0) == -1){
	  perror("client send error\n");
	  exit(1);
	}

	//lock
	while(1) { //get list of current users
	  if(command_messages.size() > 0) {
	    curr_message = command_messages[0];
	    command_messages.erase(command_messages.begin());
	    break;
	  }  
	}	
	//print out current users
	cout << "Here are the current users online: \n" << curr_message << endl; 
        
	//prompt user for username
	cout << "Please enter the user you would like to chat with >> ";
	bzero((char *)& username, sizeof(username));
	getline(cin, target_user);

	//send username
	if(send(s, target_user.c_str(), strlen(target_user.c_str()) + 1, 0) == -1){
	  perror("client send error\n");
	  exit(1);
	}
        
	//prompt for message to be sent
	cout << "Please enter the message you would like to send>> ";
	getline(cin, message_to_send);
        
	//send message
	if(send(s, message_to_send.c_str(), strlen(message_to_send.c_str()) + 1, 0) == -1){
	  perror("client send error\n");
	  exit(1);
	}
        
	//receive confirmation from server_msg
	//lock
	while(1) { //get list of current users
	  if(command_messages.size() > 0) {
	    curr_message = command_messages[0];
	    command_messages.erase(command_messages.begin());
	    //cout << curr_message;
	    break;
	  }
	}
	cout << curr_message << endl;
        continue;
      } 


      else if(strcmp(message_type.c_str(), "B") == 0){
	string confirmation_message, curr_message;
	// send message type
	if(send(s, message_type.c_str(), strlen(message_type.c_str()) + 1, 0) == -1){
	  perror("client send error\n");
	  exit(1);
	}
	// receive ack from server
       	bzero((char *)& buf, sizeof(buf));
	while(1){
	  if(command_messages.size() > 0){
	    curr_message = command_messages[0];
	    command_messages.erase(command_messages.begin());
	    break;
	  }
	}
	
	//prompt for message to be sent
	cout << curr_message << ">> ";
	getline(cin, message_to_send);
	//send message
	if(send(s, message_to_send.c_str(), strlen(message_to_send.c_str()) + 1, 0) == -1){
	  perror("client send error\n");
	  exit(1);
	}
	
	// receive and print confirmation
	/*bzero((char *)& confirmation_message, sizeof(confirmation_message));
	if(recv(s, buf, sizeof(buf) + 1, 0) == -1) {
	  perror("client receive error");
	  exit(1);
	  }*/
	while(1){
	  if(command_messages.size() > 0){
	    curr_message = command_messages[0];
	    command_messages.erase(command_messages.begin());
	    break;
	  }
	}
	cout << curr_message << endl;
	continue;
      } else if(strcmp(message_type.c_str(), "E")== 0){
	if(send(s, message_type.c_str(), strlen(message_type.c_str()) + 1, 0) == -1) {
	  perror("client send error\n");
	}
	pthread_join(thread, NULL);
	if(close(s) != 0) {
	  printf("Socket was not closed\n");
	}
	break;
      } else {
	printf("Invalid Entry\n");
      }         
    }
    //Exit
    /*pthread_join(thread, NULL);
    if(close(s) != 0) {
      printf("Socket was not closed\n");
      }*/
}

//determine if data or command message
void *handle_messages(void *socket_desc){
  string message;
  string user = "temp user";
  while(1) {
    bzero((char *)& buf, sizeof(buf));
      if(recv(s, buf, sizeof(buf) + 1, 0) == -1){
        perror("Client receive error\n");
        //exit(1);
    }
    message = buf;
    //send command messages to command vector and print messages from other clients
    if(message.at(0) == 'C') {
      message.erase(0,1);
      command_messages.push_back(message); //lock
    } else {
      message.erase(0,1);
      cout << "#### New Message: Message Received from " << user << ": " << message << " ####" << endl;
      prompt_for_input();
    }
  }
}

void prompt_for_input(){
  printf("Enter P for private conversation. Enter B for message broadcasting. Enter E to exit. >> ");
}

void private_message(){
  //return;
}

void broadcast(){
  //return;
}

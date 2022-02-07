/**
 * Author: Bronson Zell
 * GT Email: bzell@gatech.edu
 */
#include <stdio.h>
#include <stdlib.h>    
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define LENGTH 200

extern char *optarg;
extern int optind, opterr, optopt;
int sockfd;
char *username;

void print_usage(){
  printf("Usage: ./chatclient_tcp -join -host <host> -port <port> -username <username> -passcode <passcode>\n");
}

void str_trim (char* arr) {
  int i;
  for (i = 0; i < LENGTH; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void shortcuts(char* source, char* substring, char* with){
  char *substring_source = strstr(source, substring);
  
  while(substring_source != NULL){
    
    memmove(
    substring_source + strlen(with),
    substring_source + strlen(substring),
    strlen(substring_source) - strlen(substring) + 1
    );

    memcpy(substring_source, with, strlen(with));

    substring_source = strstr(source, substring);
  }
}

char *get_time(){

  time_t rawtime;

  time(&rawtime);
  char *time = ctime(&rawtime);
  str_trim(time);

  return time;
}

char *get_plus_time(){
  time_t rawtime;

  time(&rawtime);
  rawtime = rawtime + 3600;
  char *time = ctime(&rawtime);
  str_trim(time);
  
  return time;
}

void *send_message_thread(void *arg){

  char message[LENGTH];
  char buffer[LENGTH + 32];

  char happy[] = ":)";
  char feeling_happy[] = "[feeling happy]";

  char sad[] = ":(";
  char feeling_sad[] = "[feeling sad]";

  char mytime[] = ":mytime";
  char plustime[] = ":+1hr";

  char exit_code[LENGTH] = ":Exit";



  while(1){
    bzero(message, LENGTH);
    bzero(buffer, LENGTH+32);
    fflush(stdout);
    fgets(message, LENGTH, stdin);
    str_trim(message);

    if(strcmp(message, "")!=0){

    if(strstr(message, ":Exit")){
      write(sockfd, exit_code, LENGTH);
      close(sockfd);
      exit(EXIT_SUCCESS);
    }

    shortcuts(message, happy, feeling_happy);
    shortcuts(message, sad, feeling_sad);
    shortcuts(message, mytime, get_time());
    shortcuts(message, plustime, get_plus_time());

    sprintf(buffer, "%s: %s", username, message);
    write(sockfd, buffer, LENGTH);
    }
  }
  return 0;
} 

void *recieve_message_thread(void *arg){

  char message[LENGTH];

  while(1){
    bzero(message, LENGTH);
    int x = read(sockfd, message, LENGTH);
    if(x > 0){
      printf("%s\n", message);
      fflush(stdout);
    }
  }
  
}
           
int main(int argc, char *argv[]) {

  int opt = 0;
  int join = 0;
  int port;
  char *host;
  
  char *passcode;

        static struct option long_options[] = {
            {"join",    no_argument,       0,  'j' },
            {"host",    required_argument, 0,  'h' },
            {"port",    required_argument, 0,  'p' },
            {"username",required_argument, 0,  'u' },
            {"passcode",required_argument, 0,  'c' },
            {0,         0,                 0,  0 }
        };
        
        int long_index = 0;
        while((opt = getopt_long_only(argc, argv,"jh:p:u:c:", long_options, &long_index)) != -1){
          switch(opt){
            case 'j' : join = 1;
              break;
            case 'h' : host = optarg;
              break;
            case 'p' : port = atoi(optarg);
              break;
            case 'u' : username = optarg;
              break;
            case 'c' : passcode = optarg;
              break;
            default: print_usage();
              exit(EXIT_FAILURE);
          }
        }

    if(join != 1){
      print_usage();
      exit(EXIT_FAILURE);
    }
        
        struct sockaddr_in client;

        //Creates Socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd == -1){
            printf("Could not create socket");
            exit(EXIT_FAILURE);
        }
        
        client.sin_addr.s_addr = inet_addr(host);
        client.sin_family = AF_INET;
        client.sin_port = htons(port);

        //Connect to client
        if (connect(sockfd, (struct sockaddr *)&client , sizeof(client)) < 0){
          puts("connect error\n");
          exit(EXIT_FAILURE);
        }

        char buff[LENGTH];
        bzero(buff, LENGTH);

        write(sockfd, passcode, LENGTH);
        read(sockfd, buff, LENGTH);
        if(strcmp(buff, "Incorrect passcode")==0){
          printf("%s", buff);
          printf("\n");
          close(sockfd);
          exit(EXIT_FAILURE);
        }
        
        printf("Connected to %s on port %d\n", host, port);

        //Sending Username and Passcode

        write(sockfd, username, 32);

        pthread_t send_message;
        if(pthread_create(&send_message, NULL, &send_message_thread, NULL) != 0){
          printf("Send message thread failed");
          close(sockfd);
          exit(EXIT_FAILURE);
        }

        pthread_t recieve_message;
        if(pthread_create(&recieve_message, NULL, &recieve_message_thread, NULL) != 0){
          printf("Recieve message thread failed");
          close(sockfd);
          exit(EXIT_FAILURE);
        }

        while(1);

  return 0;
}
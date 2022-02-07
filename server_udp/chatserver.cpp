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

#define MAX_CLIENTS 20
#define LENGTH 200

extern char *optarg;
extern int optind, opterr, optopt;
char *passcode;
int serverfd;
int id = 1;


typedef struct{
    char name[32];
    struct sockaddr_in addr;
    int id;
} client_struct;

client_struct *clients[MAX_CLIENTS];

void print_usage(){
	printf("Usage: ./chatserver_udp -start -port <port> -passcode <passcode>\n");
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

void add_client(client_struct *cl){

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}
}

void remove_client(int id){

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->id == id){
				clients[i] = NULL;
				break;
			}
		}
	}

}

void list_clients(){
    for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i]){
            printf("%s ", clients[i]->name);
        }
    }
    printf("\n");
}

void *server_handler(void *arg){
    char message[LENGTH];
	char buffer[LENGTH + 32];

  while(1) {
    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
    fflush(stdout);
    fgets(message, LENGTH, stdin);
    str_trim(message);

    if (strcmp(message, ":Exit") == 0) {
            close(serverfd);
            exit(EXIT_SUCCESS);
    } else if(strcmp(message, "listclients") == 0){
            list_clients();
    } 
  }
}

void send_message(int id, char *message){

    for(int i = 0; i<MAX_CLIENTS; i++){
        if((clients[i])&&(clients[i]->id != id)){
            socklen_t len = sizeof(clients[i]->addr);
            if(sendto(serverfd, message, LENGTH, 0, (struct sockaddr*)&clients[i]->addr, len)<0){
                puts("Error sending message");
            }
        }
    }
}

void *client_connected(struct sockaddr_in client_addr){

    for(int i=0; i<MAX_CLIENTS; i++){
        if( (clients[i]) && (ntohs(clients[i]->addr.sin_port) == ntohs(client_addr.sin_port)) && (strcmp(inet_ntoa(clients[i]->addr.sin_addr),inet_ntoa(client_addr.sin_addr))==0)){
            return clients[i];
        }
    }

return NULL;
}

char *parse_pass(char message[LENGTH]){

    int i = 0;
    char *pass_parsed = (char *)malloc(LENGTH);

    while(message[i]!='\n'){
        pass_parsed[i] = message[i];
        i++;
    }

    return pass_parsed;
}

char *parse_name(char message[LENGTH]){

    char *name_parsed = (char *)malloc(LENGTH);

    for(int i=0; i<LENGTH; i++){
        if(message[i]=='\n'){
            i++;
            for(int x = i; x<LENGTH; x++){
                name_parsed[x-i] = message[x];
            }
            return name_parsed;
        }
    }
    return 0;
}

void *connect_client(struct sockaddr_in client_addr, char message[LENGTH]){

    char buff[LENGTH];
    bzero(buff, LENGTH);
    socklen_t client_addr_length = sizeof(client_addr);

    if(strcmp(passcode, parse_pass(message)) != 0){

        sprintf(buff, "Incorrect passcode");
        sendto(serverfd, buff, LENGTH, 0, (struct sockaddr*)&client_addr, client_addr_length);

        return NULL;

    }else{
        sprintf(buff, "Correct passcode");
        sendto(serverfd, buff, LENGTH, 0, (struct sockaddr*)&client_addr, client_addr_length);
    }

    client_struct *cli = (client_struct *)malloc(sizeof(client_struct));
    cli->addr = client_addr;
    cli->id = id++;
    sprintf(cli->name, "%s", parse_name(message));
    add_client(cli);

    sprintf(buff, "%s joined the chatroom", cli->name);
    printf("%s\n",buff);
    send_message(cli->id, buff);

return cli;
}

int main(int argc, char *argv[]) {

	int opt = 0;
	int start = 0;
	int port;
    struct sockaddr_in server;
    

        //Parse inputs
        static struct option long_options[] = {
            {"start",   no_argument,       0,  's' },
            {"port",    required_argument, 0,  'p' },
            {"passcode",required_argument, 0,  'c' },
            {0,         0,                 0,  0 }
        };
        
        int long_index = 0;
        while((opt = getopt_long_only(argc, argv,"", long_options, &long_index)) != -1){
        	switch(opt){
        		case 's' : 
        			start = 1;
        			break;
        		case 'p' :
        			port = atoi(optarg);
        			break;
        		case 'c' : 
        			passcode = optarg;
        			break;
        		default: print_usage();
        			exit(EXIT_FAILURE);
        	}
        }
		if(start == 0){
			print_usage();
			exit(EXIT_FAILURE);
		}

        //Creates Socket
        serverfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (serverfd == -1){
            printf("Could not create socket");
            exit(EXIT_FAILURE);
        }

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);

        //Bind
        if( bind(serverfd,(struct sockaddr *)&server , sizeof(server)) < 0){
        puts("bind failed");
        exit(EXIT_FAILURE);
        }

        pthread_t server_t;

        if(pthread_create(&server_t, NULL, &server_handler, NULL) != 0){
		    puts("ERROR: pthread");
            return EXIT_FAILURE;
	    }

        //Accept incoming connections
        printf("Server started on port %d. Accepting connections\n", port);

        char message[LENGTH];
        char buff[LENGTH];

        while(1){

            bzero(message, LENGTH);
            bzero(buff, LENGTH);

            struct sockaddr_in client_addr;
            socklen_t client_addr_length = sizeof(client_addr);

            recvfrom(serverfd, message, LENGTH, 0, (struct sockaddr*)&client_addr, &client_addr_length);
            if(client_connected(client_addr)==0){
                connect_client(client_addr, message);
            } else{

                client_struct *cli = (client_struct *)client_connected(client_addr);

                if(strcmp(message, ":Exit") == 0){
                    sprintf(buff, "%s left the chatroom", cli->name);
                    printf("%s\n", buff);
                    send_message(cli->id, buff);
                    remove_client(cli->id);
                    free(cli);
                }else{
                    send_message(cli->id,message);
                    printf("%s\n", message);
                }

                
            }

        }

        return 0;

}
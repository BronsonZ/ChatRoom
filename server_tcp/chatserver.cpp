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
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int serverfd;

typedef struct{
    char name[32];
    int sockfd;
    struct sockaddr_in addr;
    int id;
} client_struct;

client_struct *clients[MAX_CLIENTS];

void print_usage(){
	printf("Usage: ./chatserver_tcp -start -port <port> -passcode <passcode>\n");
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
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int id){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->id == id){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void list_clients(){
    for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i]){
            printf("%s ", clients[i]->name);
        }
    }
    printf("\n");
}

void disconnect_clients(){
    char message[LENGTH];
    for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i]){
            sprintf(message, "Server Closed");
            write(clients[i]->sockfd, message, LENGTH);
            close(clients[i]->sockfd);
        }
    }
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
            disconnect_clients();
            close(serverfd);
            exit(EXIT_SUCCESS);
    } else if(strcmp(message, "listclients") == 0){
            list_clients();
    } 
  }
}

void send_message(int id, char *message){
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i<MAX_CLIENTS; i++){
        if((clients[i])&&(clients[i]->id != id)){
            if(write(clients[i]->sockfd, message, LENGTH)<0){
                puts("Error sending message");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *connection_handler(void *arg){


    client_struct *cli = (client_struct *)arg;
    char pass[LENGTH];
    char buff[LENGTH];
    bzero(pass, LENGTH);
    bzero(buff, LENGTH);

    //Get Passcode
    if(read(cli->sockfd, pass, LENGTH) == -1 ){
        puts("passcode recv failed");

        close(cli->sockfd);
        free(cli);
        pthread_detach(pthread_self());

        return 0;
    }
    //Check Passcode
    if(strcmp(passcode, pass) != 0){

        sprintf(buff, "Incorrect passcode");
        write(cli->sockfd, buff, LENGTH);

        close(cli->sockfd);
        free(cli);
        pthread_detach(pthread_self());

        return 0;
    }
    
    sprintf(buff, "Correct Passcode");
    write(cli->sockfd, buff, LENGTH);

    //Get Username
    if(read(cli->sockfd, cli->name, 32) == -1){
        puts("Username recv failed");

        close(cli->sockfd);
        free(cli);
        pthread_detach(pthread_self());

        return 0;
    }

    add_client(cli);

    bzero(buff, LENGTH);

    sprintf(buff, "%s joined the chatroom", cli->name);
    printf("%s\n",buff);
    send_message(cli->id, buff);

    //Receive client messages
    while(1){
        bzero(buff, LENGTH);
        int x = read(cli->sockfd, buff, LENGTH);
        str_trim(buff);
        
        if(x == -1) {
            puts("Recv error in conneciton handler");
        } else if(strcmp(buff, ":Exit") == 0){
            sprintf(buff, "%s left the chatroom", cli->name);
            printf("%s\n", buff);
            send_message(cli->id, buff);

            remove_client(cli->id);
            close(cli->sockfd);
            free(cli);
            pthread_detach(pthread_self());
            break;
        } else if(x>0){
            send_message(cli->id, buff);
            printf("%s\n", buff);
        } 
    }
    return 0;
}

int main(int argc, char *argv[]) {

	int opt = 0;
	int start = 0;
    int id = 0;
	int port;
    struct sockaddr_in server, client_addr;

        //Sparse inputs
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
        serverfd = socket(AF_INET, SOCK_STREAM, 0);

        if (serverfd == -1){
            printf("Could not create socket");
            exit(EXIT_FAILURE);
        }

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);

        //Bind
        if( bind(serverfd,(struct sockaddr *)&server , sizeof(server)) < 0)
        {
        puts("bind failed");
        exit(EXIT_FAILURE);
        }

        //Listen
        if(listen(serverfd, 5) == -1){
            puts("Error at listen()");
            exit(EXIT_FAILURE);
        }

        //Accept incoming connections
        printf("Server started on port %d. Accepting connections\n", port);

        pthread_t server_t;
        if(pthread_create(&server_t, NULL, &server_handler, NULL) != 0){
		    puts("ERROR: pthread");
        return EXIT_FAILURE;
	    }


        while(1){

            pthread_t client_thread;
            
            socklen_t c = sizeof(client_addr);
            int clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &c);

            if(clientfd == -1){
                puts("Error accpeting client");
            }

            client_struct *cli = (client_struct *)malloc(sizeof(client_struct));
            cli->addr = client_addr;
            cli->sockfd = clientfd;
            cli->id = id++;
            
            if(pthread_create(&client_thread, NULL, &connection_handler, (void*)cli) != 0){
                puts("Error creating client thread");
            }
        }

        return 0;

}
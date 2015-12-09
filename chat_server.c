#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>    
#define MAX_CLIENTS	100

static unsigned int client_count = 0;
static int uid = 10;

/* Client structure */
typedef struct {
	struct sockaddr_in addr;	/* Client remote address */
	int connfd;			/* Connection file descriptor */
	int uid;			/* Client unique identifier */
	char name[32];			/* Client name */
} client_t;

client_t *clients[MAX_CLIENTS];


//the thread function
void *client_handler(void *);


/* Add client to queue */
void queue_add(client_t *cl){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(!clients[i]){
			clients[i] = cl;
			return;
		}
	}
}

/* Send message to all clients but the sender */
void send_message(char *s, int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid != uid){
				write(clients[i]->connfd, s, strlen(s));
			}
		}
	}
}


/* Delete client from queue */
void queue_delete(int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				return;
			}
		}
	}
}


void *client_handler(void *arg)
{	

	puts("Handler assigned");
	client_count++;
	client_t *cli = (client_t *)arg;
	printf("<<ACCEPT ");
	printf(" REFERENCED BY %d\n", cli->uid);
    
   
    int read_size;
    char *message , client_message[2000];
     
    

    //Send some messages to the client
    message = "connection handler is ready\n";
    write(cli->connfd, message , strlen(message));
     
    message = "server will repeat client message \n";
    write(cli->connfd , message , strlen(message));
     
    //Receive a message from client
    while( (read_size = recv(cli->connfd , client_message , 2000 , 0)) > 0 )
    {
        //end of string marker
		client_message[read_size] = '\0';

		puts(client_message);
		//Send the message back to client
        write(cli->connfd , client_message , strlen(client_message));
		
		send_message(client_message, cli->uid);


		//clear the message buffer
		memset(client_message, 0, 2000);


    }


    /* Close connection */
	close(cli->connfd);
	sprintf(client_message, "<<LEAVE, BYE %s\r\n", cli->name);
	send_message(client_message,cli->uid);

	/* Delete client from queue*/
	queue_delete(cli->uid);
	printf("<<LEAVE ");
	printf(" REFERENCED BY %d\n", cli->uid);
	free(cli);
	client_count--;

    return 0;

}	





int main(int argc, char *argv[])

{
	/* Variables */
	int sock, mysock, connection;
	struct sockaddr_in server, client;
	
	char buff[1024];
	
	pthread_t thread_id;


	/* Creat Socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		perror("Failed to creat Socket");
		exit(1);
	}

	/* Socket Settings*/
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(5000)		;

	/* Call Bind */

	if(bind(sock, (struct sockaddr *)&server, sizeof(server))<0)
	{
		perror("Bind Failed");
	}

	/* Listen */

	listen(sock,5);

	puts("ready for listening and Waiting for incoming connections\n");

	/* Accept */
	while(1)
	{	
		socklen_t connection = sizeof(client);
		mysock = accept(sock, (struct sockaddr *)&client, &connection);

		puts("Connection accepted");

		if(mysock == -1)
		{
			perror("accept failed");		
		}
		
		else
		{	


		/* Check if max clients is reached */
		if((client_count+1) == MAX_CLIENTS)
			{
				printf("<<MAX CLIENTS REACHED\n");
				printf("<<REJECT ");
				close(mysock);
				continue;
			}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = client;
		cli->connfd = mysock;
		cli->uid = uid++;
		sprintf(cli->name, "%d", cli->uid);	


		queue_add(cli);
		/* process each t */
		pthread_create(&thread_id, NULL, client_handler, (void *)cli);
		pthread_detach(thread_id);
		}

	}

	
	return 0;

}
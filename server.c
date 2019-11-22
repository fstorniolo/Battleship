#include "library/library.h"

struct client_connected{
	int UDP_port, socket_TCP;
	char *username;
	int state;
	char ip[INET_ADDRSTRLEN];
	struct client_connected* next;
	struct client_connected* pending_connection;
	struct client_connected* in_game_with;

};


struct client_connected *client_connected_list=NULL;
struct client_connected *client_connected_list_back=NULL; 
int num_client_connected=0;
fd_set master;



struct client_connected* search_in_client_list_by_username(char* username_client){
	struct  client_connected *my_list = client_connected_list;
	while(my_list != NULL ){
		if(!strcmp(my_list->username,username_client))
			return my_list;
		else
			my_list =  my_list->next;
	}
	return my_list;
}	

struct client_connected* search_in_client_list_by_socket(int my_socket){
	struct  client_connected *my_list = client_connected_list;
	while(my_list != NULL ){
		if(my_list->socket_TCP == my_socket)
			return my_list;
		else
			my_list =  my_list->next;
	}
	return my_list;
}	

bool store_client(char* client, int UDP_Port, int socket_client, struct sockaddr_in cl_addr){
    struct client_connected *my_list=client_connected_list;
    struct client_connected *obj_client;
    bool already_used=false;
	
    //if(client_connected_list != NULL){
	while(my_list != NULL){
		if(already_used)
			break;
		//printf("Sono nel while stampo %s e %s",client,my_list->username);
		if(!strcmp(client,my_list->username)){
		    already_used=true;
		 //   printf("already_used è true\n");
		    
 		}
		else{
		//    printf("sono in my_list next\n");    	
		    my_list=my_list->next;

		}
    	}
   //}

 //  printf("Ho finito la if\n");
    
  
   if(already_used){
	printf("è già usato\n");
	return false;
	}
	
   obj_client = (struct client_connected *)malloc(sizeof(struct client_connected));
   obj_client->username = (char *)malloc(strlen(client));
   obj_client->next=NULL;
   obj_client->state = FREE;
   obj_client->UDP_port = UDP_Port;
   obj_client->socket_TCP = socket_client;
   obj_client->pending_connection = NULL;
   obj_client->in_game_with = NULL;
   inet_ntop(AF_INET,&cl_addr.sin_addr,obj_client->ip,INET_ADDRSTRLEN);		//converto l'ip in stringa
   strcpy(obj_client->username, client);
	
   if(!client_connected_list){
	client_connected_list = client_connected_list_back= obj_client;
	//printf("Inserisco il primo utente: funzione store_client\n");
    }
   else{
   	client_connected_list_back->next = obj_client;
	client_connected_list_back = client_connected_list_back->next;
	//printf("Inserisco un altro utente: funzione store_client\n");
    }


	
   num_client_connected++;
   return true;
  
}

//SERVER
void who_command(int new_sd){
	//printf("Eseguo who\n");
	int i,current_username_state,ret;
	char *current_username;
	struct client_connected* my_list=client_connected_list;
	sendIntTCP(new_sd,num_client_connected);
	for(i = 0; i < num_client_connected; i++){
		current_username = (char *)malloc(strlen(my_list->username));
		strcpy(current_username,my_list->username);
		current_username_state=my_list->state;
		ret=sendStringTCP(new_sd,current_username);
		if(!ret)
			return;
		if(!ret)
			return;
		ret=sendIntTCP(new_sd,current_username_state);
		free(current_username);
		my_list=my_list->next;
	}
}
		
		
void initialize_server(int* socket_server, int server_port){

    int ret;
    struct sockaddr_in sv_addr;
    *socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if(*socket_server < 0){
	printf("Errore nella creazione del socket\n");
	exit(-1);
    }

   
    memset(&sv_addr,0, sizeof(sv_addr));
    sv_addr.sin_family=AF_INET;
    sv_addr.sin_port = htons(server_port);
    ret=inet_pton(AF_INET,"0.0.0.0",&sv_addr.sin_addr);
    if(ret < 0){
	printf("Errore in inet_pton\n");
	exit(-1);
    }

    ret = bind(*socket_server, (struct sockaddr* )&sv_addr, sizeof(sv_addr));
    if(ret < 0){
	printf("Errore in bind\n");
	exit(-1);
   }

    printf("Indirizzo: %s porta (%d) \n","127.0.0.1",server_port);
    ret = listen(*socket_server,10);
    if(ret < 0){
	printf("Errore nella listen\n");
	exit(-1);
    }
}

void remove_user(int socket_client){

	bool prev = false;	
	
	struct client_connected* my_list = client_connected_list;
	struct client_connected* obj_prev = client_connected_list;
	while(my_list != NULL){
		if(my_list->socket_TCP == socket_client){
			if(prev){
				obj_prev->next = my_list->next;
				if(my_list->next == NULL)
					client_connected_list_back = obj_prev;

			}else{
				client_connected_list = client_connected_list->next;
				if(my_list->next == NULL)
					client_connected_list_back = NULL;
	
			}
			printf("\n%s si è disconnesso\n",my_list->username);
			free(my_list->username);
			free(my_list);
			num_client_connected--;
			return;
	
		}

		else{
			if(!prev){
				prev = true;
				my_list = my_list->next;
			}else{
				my_list=my_list->next;
				obj_prev = obj_prev->next;
			} 
		}
		
	}
	close(socket_client);

}

void quit_command(int socket_client){
	
	remove_user(socket_client);
	close(socket_client);
	FD_CLR(socket_client,&master);

}

void clean_players_were_in_connection(int socket_client_disconnected){
struct client_connected *client_disconnected;

	client_disconnected = search_in_client_list_by_socket(socket_client_disconnected);
	client_disconnected->state = FREE;
	client_disconnected->pending_connection->state = FREE;
	client_disconnected->pending_connection->pending_connection = NULL;
	client_disconnected->pending_connection = NULL;

}

void clean_players_were_in_game(int socket_client_disconnected){
	struct client_connected *client_disconnected;

	client_disconnected = search_in_client_list_by_socket(socket_client_disconnected);
	client_disconnected->state = FREE;
	client_disconnected->in_game_with->state = FREE;
	client_disconnected->in_game_with->in_game_with = NULL;
	client_disconnected->in_game_with = NULL;
}

void disconnect_command(int socket_client_disconnected){
	int ret;
	struct client_connected *client_disconnected;

	client_disconnected = search_in_client_list_by_socket(socket_client_disconnected);
	ret = sendIntTCP(client_disconnected->in_game_with->socket_TCP,DISCONNECT_COMMAND);
	if(!ret)
		return;
	
	clean_players_were_in_game(socket_client_disconnected);
	return;

}


void login_command(int new_sd, struct sockaddr_in cl_addr){
    int UDP_Port, ret;
    char* recvString;
    bool bool_ret = false;

    recvString = recvStringTCP(new_sd);
    ret = recvIntTCP(new_sd,&UDP_Port);

    if(!ret || recvString == NULL){
	printf("Si è verificato un errore nel login\n");
	sendIntTCP(new_sd,LOGIN_FAIL);
	free(recvString);
        return;
    }

    bool_ret = store_client(recvString,UDP_Port, new_sd, cl_addr);

    if(bool_ret){
	printf("\n%s si è connesso\n",recvString);
	printf("%s è libero \n",recvString);
   	sendIntTCP(new_sd,LOGIN_OK);
   }
    else{
	 printf("login fallito da client\n");
	 sendIntTCP(new_sd,LOGIN_FAIL);
	}
		
}

int control_username(struct client_connected* challenger,  char* username, int* opponent_port, char* ip, int* opponent_socket){
	
	struct client_connected *my_list = client_connected_list;	

	while(my_list != NULL){
		if(!strcmp(username,my_list->username)){
			if(my_list->state == BUSY || my_list->state == CONNECTING)
				return my_list->state;
			if(my_list->state == FREE){
				*opponent_port = my_list->UDP_port;
				*opponent_socket = my_list->socket_TCP;
				strcpy(ip,my_list->ip);
				my_list->state = CONNECTING;
				my_list->pending_connection = challenger;
				challenger->pending_connection = my_list;
				challenger->state = CONNECTING;
				return FREE;
			}
		}else
			my_list = my_list->next;
	}
	return NO_USERNAME;

}

void contact_opponent( int opponent_socket, char* username_client){
	
	int ret;

	printf("invio la richiesta all'avversario\n");
	ret = sendIntTCP(opponent_socket,MATCH_REQUEST);
	if(!ret)
		return; //CONNECT_FAIL;

	ret = sendStringTCP(opponent_socket,username_client);
	if(!ret)
		return; // CONNECT_FAIL;
	//free(username_client);
	return; // WAITING_FOR_ANSWER;
	
}
	



void connect_command(int client_socket){
	struct client_connected *tmp;
	char* username_opponent;
	char* username_client;
	int ret, opponent_port, opponent_socket;
	char ip[INET_ADDRSTRLEN];
	
	username_client = recvStringTCP(client_socket);
	tmp = search_in_client_list_by_username(username_client);
	username_opponent = recvStringTCP(client_socket);
	printf("Ho ricevuto l'username %s , sono dentro la connect\n", username_opponent);
	ret = control_username(tmp,username_opponent,&opponent_port,ip, &opponent_socket);
	
	 if(ret == BUSY || ret == CONNECTING || ret == NO_USERNAME){
		sendIntTCP(client_socket, ret);
		return;
	}							
	else{				
		contact_opponent(opponent_socket,username_client);																				//username is free
		//printf("contatto l'avversario %s\n",username_opponent);	
	}
	return;
}




void send_game_answer(int opponent_socket, int cmd){
	int challenger_socket, ret;
	struct client_connected* client_opponent;
	client_opponent = search_in_client_list_by_socket(opponent_socket);
/*
	if(cmd == MATCH_ACCEPTED)
		client_opponent->state = client_opponent->pending_connection->state = BUSY;
	if(cmd == MATCH_REFUSED)
		client_opponent->state = client_opponent->pending_connection->state = FREE;*/

	challenger_socket = client_opponent->pending_connection->socket_TCP;
	//client_opponent->pending_connection = NULL;
	ret = sendIntTCP(challenger_socket,cmd);
	if(!ret)
		return;
	if(cmd == MATCH_ACCEPTED){
		ret = sendStringTCP(challenger_socket, client_opponent->ip);
		if(!ret)
			return;
		ret = sendIntTCP(challenger_socket, client_opponent->UDP_port);
		if(!ret)
			return;
		ret = sendStringTCP(opponent_socket, client_opponent->pending_connection->ip);
		if(!ret)
			return;
		ret = sendIntTCP(opponent_socket, client_opponent->pending_connection->UDP_port);
		if(!ret)
			return;

		client_opponent->state = client_opponent->pending_connection->state = BUSY;
		client_opponent->in_game_with = client_opponent->pending_connection;
		client_opponent->pending_connection->in_game_with = client_opponent;


	}else{

		client_opponent->state = client_opponent->pending_connection->state = FREE;

	}

	client_opponent->pending_connection->pending_connection = NULL;
	client_opponent->pending_connection = NULL;


}

void timeout_in_game(int client_socket){
	struct client_connected *client_timeout;
	int ret;
	int opponent_socket;
	client_timeout = search_in_client_list_by_socket(client_socket);
	opponent_socket = client_timeout->in_game_with->socket_TCP;
	ret = sendIntTCP(opponent_socket,OPPONENT_TIMEOUT_IN_GAME);
	if(!ret)
		return;
	clean_players_were_in_game(client_socket);


}
void timeout_in_connection(int client_socket){
	struct client_connected *client_timeout;
	int ret;
	int opponent_socket;
	client_timeout = search_in_client_list_by_socket(client_socket);
	opponent_socket = client_timeout->pending_connection->socket_TCP;
	ret = sendIntTCP(opponent_socket,TIMEOUT_IN_CONNECTION);
	if(!ret)
		return;
	clean_players_were_in_connection(client_socket);


}

void which_command_is(int client_socket, int cmd, struct sockaddr_in cl_addr){
	switch(cmd){
		case(WHO_COMMAND):
	    	who_command(client_socket);
			return;
		case(LOGIN_COMMAND):
			login_command(client_socket,cl_addr);
			return;
		case(QUIT_COMMAND):
			quit_command(client_socket);
			return;
		case(CONNECT_COMMAND):
			connect_command(client_socket);
			return;
		case(MATCH_ACCEPTED):
			send_game_answer(client_socket, cmd);
			return;
		case(MATCH_REFUSED):
			send_game_answer(client_socket, cmd);
			return;
		case(DISCONNECT_COMMAND):
			disconnect_command(client_socket);
			return;
		case(I_HAVE_WON):
			clean_players_were_in_game(client_socket);
			return;
		case(TIMEOUT_IN_GAME):
			timeout_in_game(client_socket);
			return;
		case(TIMEOUT_IN_CONNECTION):
			timeout_in_connection(client_socket);
			return;
	}

	printf("Nessuna delle altre\n");
			
}

void error_client(int client_socket){
	printf("sono in error client");
	struct client_connected *client_connected_error;
	client_connected_error = search_in_client_list_by_socket(client_socket);
	if(client_connected_error == NULL){
		printf("client_connected_error è NULL\n");
		FD_CLR(client_socket,&master);
		return;
	}
	switch(client_connected_error->state){
		case(FREE):
			printf("L'utente %s si è disconnesso mentre era libero \n",client_connected_error->username);
			remove_user(client_socket);
			//printf("Sono in error_client, blocco FREE\n\n");
			FD_CLR(client_socket,&master);
			return;
		case(CONNECTING):
			//printf("L'utente si disconnette mentre è in connecting\n\n");
			sendIntTCP(client_connected_error->pending_connection->socket_TCP,CLIENT_DISCONNECTION);
			clean_players_were_in_connection(client_socket);
			remove_user(client_socket);
			FD_CLR(client_socket,&master);
			return;
		case(BUSY):
			//printf("L'utente si disconnette mentre è in game\n\n");
			sendIntTCP(client_connected_error->in_game_with->socket_TCP,DISCONNECT_COMMAND);
			clean_players_were_in_game(client_socket);
			remove_user(client_socket);
			FD_CLR(client_socket,&master);
			return;
	}
}

int main(int argc, char *argv[]){

    fd_set read_fds;
    int listener, new_sd, len,ret, server_port, fdmax, i,cmd;
    struct sockaddr_in cl_addr;

	
    if(argc < 2){
	printf("Parametro assente \n");
	return 0;
	}
	
    server_port = atoi(argv[1]);

    initialize_server(&listener,server_port);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(listener,&master);

    fdmax = listener;
    for(;;){
		read_fds = master;
		select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		for(i = 0; i < fdmax + 1; i++){
		    if(FD_ISSET(i, &read_fds)){
				if(i ==  listener){	
					//printf("Select, nuovo utente\n");
					memset(&cl_addr,0,sizeof(cl_addr));	
					len = sizeof(cl_addr);
		    		new_sd = accept(listener, (struct sockaddr* )&cl_addr, (socklen_t*)&len);
  				//  printf("%d",new_sd);
    				if(new_sd == -1){
    				    printf("Errore nell'accettazione\n");
    				//return 0;
    		    	}

					FD_SET(new_sd, &master);
					fdmax = (new_sd > fdmax)?new_sd:fdmax;			
    				printf("\nConnessione stabilita con il client\n");

				}else{
				//printf("Select, utente registrato manda un cmd\n");			
					ret = recvIntTCP(i,&cmd);
					if(!ret){
						printf("Client si disconnette\n");
						 error_client(i);

						//printf("Errore nella receive\n");
				//return 0;
					}else{
			//printf("Ricevo dati dal client\n");
					which_command_is(i,cmd,cl_addr);
					}
				}
	    	}
		}
    }
    close(listener);
}
		



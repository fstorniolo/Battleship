#include "library/library.h"

fd_set master;
int port_UDP, socket_server, socket_client, opponent_port;
char* my_username;
char* opponent_ip;
bool in_game;
bool my_turn;
char my_grid[SIDE][SIDE];
char opponent_grid[SIDE][SIDE];
int opponent_ships_left = SHIPS;
int my_ships_left =  SHIPS;
struct sockaddr_in opponent_sockaddr;
bool opponent_ready;
bool have_to_response_to_connection;
struct timeval timeout_main = {TIMEOUT,0};

void response_to_connection();

struct sockaddr_in initialize_opponent_udp(){
	struct sockaddr_in sv_addr;
    memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(opponent_port);
	inet_pton(AF_INET,opponent_ip,&sv_addr.sin_addr);
	return sv_addr;
}

void clean_buff(){
	char c;
	while((c = getchar()) != '\n' && c != EOF);

}

void make_grids(){
	opponent_ships_left = my_ships_left = SHIPS;
	int i,j, remaining_ships = SHIPS;
	char* readCoordinate;										// ~ = mia/sua acqua, ? = sconosciuto, x = colpito sua nave/mia nave, - = mia nave distrutta
	for(i = 0; i < SIDE; i++){
		for(j = 0; j < SIDE; j++){
			opponent_grid[i][j] = '?';
			my_grid[i][j] = '~';
		}

	}
	printf("Selezione %d caselle nelle quali inserire le navi \n",SHIPS);
	while(true){
		printf("Inserisci una nave in formato carattere/numero (es A1):\n ");
		scanf("%ms",&readCoordinate);
		if(strlen(readCoordinate)==2){
			i = (readCoordinate[0] - 'A');
			j = (readCoordinate[1] - '1');
			if(i >= SIDE || j >= SIDE || i < 0 || j < 0){
				printf("Si è verificato un errore nell'inserimento della nave \n");
			}
			else{
				if(my_grid[i][j] == '~'){
					remaining_ships --;
					my_grid[i][j] = 'x';
					printf("Nave posizionata con successo, ti rimangono %d navi da posizionare! \n", remaining_ships);
					if(remaining_ships == 0){
						printf("Hai posizionato tutte le navi\n");
						sendIntUDP(socket_client,&opponent_sockaddr,I_AM_READY);
						if(my_turn == false)
							printf("Inizia l'avversario, attendi il tuo turno\n");
						else{
							printf("E' il tuo turno\n");
							timeout_main.tv_sec = TIMEOUT;
						}

						free(readCoordinate);
						return;
					}
				}else
					printf("Hai già posizionato una nave qui\n");				
				

			}
		}else
			printf("Si è verificato un errore nell'inserimento della nave \n");
	}
	free(readCoordinate);
}

void show_command(){
	int i,j, column = 1;
	char row='A';
	printf("\n Questa è la tua griglia, ti rimangono %d navi\n",my_ships_left);
	printf(" ");
	for(i = 0; i < SIDE; i++)
		printf(" %d",column + i);
	for(i = 0; i < SIDE; i++){
		printf("\n%c ",row + i);
		for (j = 0; j < SIDE; j++){
			printf("%c ",my_grid[i][j]);

		}
	}
	printf("\n");
	column = 1;
	row = 'A';
	printf("\n Questa è la griglia del tuo avversario, ti rimangono %d navi da abbattere\n",opponent_ships_left);
	printf(" ");
	for(i = 0; i < SIDE; i++)
		printf(" %d",column + i);
	for(i = 0; i < SIDE; i++){
		printf("\n%c ",row + i);
		for (j = 0; j < SIDE; j++){
			printf("%c ",opponent_grid[i][j]);

		}
	}
	printf("\n");

}

void shot_command(){
	char* readCoordinates;
	int row,column;
	if(!opponent_ready){
		printf("Non puoi colpire nessuna casella perché il tuo avversario non ha ancora terminato di inserire le navi\n");
		return;
	}

	if(!my_turn){
		printf("Non puoi colpire nessuna casella, non è il tuo turno\n");
		return;
	
	}
	scanf("%ms",&readCoordinates);

	if(strlen(readCoordinates) != 2){
		printf("La casella da colpire deve avere formato carattere/numero es:(A1)\n");
		return;
	}
	row = (readCoordinates[0] - 'A');
	column = (readCoordinates[1] - '1');
	
	if(row > SIDE || column > SIDE || row < 0 || column < 0){		
		printf("La casella da colpire deve avere formato carattere/numero es:(A1)\n");
		return;
	}

	if(opponent_grid[row][column] != '?'){
		printf("Hai già colpito questa casella, selezionane un'altra!\n");
		return;
	}

	my_turn = false;
	sendIntUDP(socket_client,&opponent_sockaddr,SHOT_COMMAND);
	sendStringUDP(socket_client,&opponent_sockaddr,readCoordinates);

}


void help_command_not_in_game(){
	printf("Sono disponibili i seguenti comandi: \n");
	printf("!help --> mostra l'elenco dei comandi disponibili \n");
	printf("!who --> mostra l'elenco dei client connessi al server \n");
	printf("!connect username --> avvia una partita con l'utente username \n");
	printf("!quit --> disconnette il client dal server \n\n");
	return;
}

void help_command_in_game(){
	printf("Sono disponibili i seguenti comandi: \n");
	printf("!help --> mostra l'elenco dei comandi disponibili \n");
	printf("!disconnect --> disconnette il client dall'attuale partita \n");
	printf("!shot square --> fai un tentativo con la casella square \n");
	printf("!show --> visualizza griglia di gioco \n\n");
	return;

}

void quit_command(){

	int ret;
	
	ret = sendIntTCP(socket_server, QUIT_COMMAND);
	if(!ret)return;
	
	close(socket_server);
	close(socket_client);
	exit(1);
}

//CLIENT
void who_command(){
	int num_username, i, current_username_state;
	char* current_username;
	sendIntTCP(socket_server,WHO_COMMAND);
	recvIntTCP(socket_server,&num_username);
	printf("\nClient connessi al Server: %d\n",num_username);
	for(i=0; i < num_username; i++){
		current_username = recvStringTCP(socket_server);
		recvIntTCP(socket_server,&current_username_state);
		if(current_username_state==FREE)
			printf("%s (%s)\n",current_username,"libero");
		else if(current_username_state == BUSY)
			printf("%s (%s)\n",current_username,"occupato");
		else
			printf("%s (%s)\n",current_username,"in connessione");
		free(current_username);
	}
	printf("\n");
}
		
int login(){		
    char* readString;
    int ret, result_server;
    bool logged_in = false, UDP_port_OK = false;

    do{
	printf("Inserisci il tuo nome: \n");
	scanf("%ms",&readString);

    	do{
		printf("Inserisci la porta UDP di ascolto: \n");
		scanf("%d",&port_UDP);
		if(port_UDP < 0){
		    printf("La porta UDP deve avere identificatore positivo\n");
		}
		else{
		    UDP_port_OK = true;
		}
	//free(readString);
   	 }while(UDP_port_OK == false);

		
	ret = sendIntTCP(socket_server,LOGIN_COMMAND);

	if(!ret){
		printf("Send di comando login non riuscita\n");
		free(readString);
		return LOGIN_FAIL;
	}
	ret = sendStringTCP(socket_server,readString);
	
	if(!ret){
		printf("Send di login non riuscita\n");
		free(readString);
		return LOGIN_FAIL;
	}
	
	ret = sendIntTCP(socket_server,port_UDP);
	if(!ret){
		printf("Send di port non riuscita\n");
		free(readString);
		return LOGIN_FAIL;
	}

	ret = recvIntTCP(socket_server,&result_server);
	if(!ret){
		printf("Receive di login non riuscita\n");
		free(readString);
		return LOGIN_FAIL;
		}
	
	if(result_server != LOGIN_OK)
		printf("Username già usato\n");
	
	else{
		//printf("Username ok \n");
		logged_in = true;
	}
    }while(!logged_in);

    my_username = readString;
    in_game = my_turn = false;
	//free(readString);
	
    
    return LOGIN_OK;
}


void disconnect_command(){
	int ret;
	ret = sendIntTCP(socket_server,DISCONNECT_COMMAND);
	if(!ret)
		return;
	printf("Ha perso, ti sei ritirato dalla partita \n");
	opponent_ready = in_game = my_turn = false;
}



int create_socket_client(){
    int ret;
    struct sockaddr_in client_addr;

    socket_client = socket(AF_INET, SOCK_DGRAM,0);
    if(socket_client < 0){
	printf("Errore durante la creazione del socket UDP\n");
	exit(-1);
    }
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port_UDP);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(socket_client,(struct sockaddr*)&client_addr,sizeof(client_addr));
	if(ret < 0){
		printf("Errore in bind\n");
		exit(-1);
	}
    return SOCKET_UDP_OK;
}

void initialize_match(){
	int ret;
	//char *prova;
	opponent_ip = recvStringTCP(socket_server);
	ret = recvIntTCP(socket_server, &opponent_port);
	if(!ret)
		return;
	opponent_sockaddr = initialize_opponent_udp();
	//ret = sendStringUDP(socket_client,&opponent_sockaddr,"Mi leggi?");
	//prova = recvStringUDP(socket_client, &opponent_sockaddr);
	//printf("Ho letto %s\n",prova);

	//printf("Stampo ip dell'avversario %s\n",opponent_ip);
	//printf("Stampo porta dell'avversario %d\n",opponent_port);
	in_game = true;
	make_grids();

}


void connect_command(){

	char* username;
	int ret, tmp;
	
	scanf("%ms",&username);
	//printf("sono in connect comand, ho letto il nome\n");
	if(!strcmp(username,my_username))
		return;
	//printf("1)invio comando al server, sono in connect \n");
	ret = sendIntTCP(socket_server,CONNECT_COMMAND);
	if(!ret)
		return;

	ret = sendStringTCP(socket_server, my_username);
	if(!ret)
		return;

	//printf("2)invio username al server, sono in connect \n");
	ret = sendStringTCP(socket_server, username);
	if(!ret)
		return;

	//printf("3)attendo risposta dal server, sono in connect \n");
	ret = recvIntTCP(socket_server, &tmp);
	if(!ret)
		return;

	printf("Sei in attesa della risposta del tuo avversario\n\n");
	
	switch(tmp){		

		case(NO_USERNAME):
			printf("Username inesistente \n");
			free(username);
			return;
		case(CONNECTING):
			printf("%s si sta connettendo con un altro giocatore \n",username);
			free(username);
			return;
		case(BUSY):
			printf("%s è già impegnato in un'altra partita \n",username);
			free(username);
			return;
		case(MATCH_ACCEPTED):
			printf("%s ha accettato di giocare con te \n",username);
			my_turn = true;
			initialize_match();	
			free(username);		
			return;
		case(MATCH_REFUSED):
			printf("%s ha rifiutato di giocare con te \n",username);
			free(username);
			return;
		case(USER_DISCONNECTED):
			printf("%s si è appena disconnesso \n", username);
			free(username);
			return;
		case(CONNECT_FAIL):
			printf("Connect fallita \n");
			free(username);
			return;
		case(CLIENT_DISCONNECTION):
			printf("Il tuo avversario si è disconnesso durante l'invito\n");
			in_game = my_turn = opponent_ready = false;
			free(username);
			return;
		case(TIMEOUT_IN_CONNECTION):
			printf("Il tuo avversario non ha risposto in tempo\n");
			in_game = my_turn = opponent_ready = false;
			free(username);
			return;

		

		
	}
}

	
void match_request(){
	//char* client_answer;
	//int ret;
	char* username_opponent;
	username_opponent = recvStringTCP(socket_server);
	have_to_response_to_connection = true;

	printf("%s ti ha invitato a giocare una partita, vuoi accettare? (y | n)\n",username_opponent );
	response_to_connection();
	free(username_opponent);
	/*do{
		scanf("%ms",&client_answer);
	}
	while(strcmp(client_answer,"y")!=0 && strcmp(client_answer,"n")!=0);

	if(!strcmp("y",client_answer)){
		ret = sendIntTCP(socket_server, MATCH_ACCEPTED);
		my_turn = false;
		initialize_match();
	}
	else
		ret = sendIntTCP(socket_server,MATCH_REFUSED);
	if(!ret)
		return;*/

}

void response_to_shot_command(){
	char* readCoordinates;
	int result, ret,row,column;
	
	readCoordinates = recvStringUDP(socket_client, &opponent_sockaddr);
	ret = recvIntUDP(socket_client,&opponent_sockaddr,&result);
	if(!ret)
		return;

	row = (readCoordinates[0] - 'A');
	column = (readCoordinates[1] - '1');

	if(result == HIT){
		opponent_grid[row][column] = 'x';
		opponent_ships_left--;
		printf("Hai colpito la nave del tuo avversario di coordinate %s, ti rimangono %d navi da abbattere\n\n",readCoordinates,opponent_ships_left);
		if(opponent_ships_left == 0){
			printf("Hai distrutto tutte le navi del tuo avversario, hai vinto!\n\n");
			sendIntTCP(socket_server,I_HAVE_WON);
			my_turn = in_game = opponent_ready = false;
			free(readCoordinates);
			return;
		}
	}else{
		opponent_grid[row][column] = '~';
		printf("Hai colpito l'acqua in coordinate %s, ti rimangono %d navi da abbattere\n\n",readCoordinates,opponent_ships_left);

	}
	printf("E' il turno del tuo avversario\n\n");
	free(readCoordinates);
	my_turn = false;



}

void have_I_been_hit(){
	char* readCoordinates;
	int row, column;
	
	readCoordinates = recvStringUDP(socket_client,&opponent_sockaddr);
	row = readCoordinates[0] - 'A';
	column = readCoordinates[1] - '1';

	if(my_grid[row][column] == 'x'){
		my_ships_left--;
		my_grid[row][column] = '-';
		printf("L'avversario ha colpita la tua nave in coordinate %s\n",readCoordinates);
		if(!sendIntUDP(socket_client,&opponent_sockaddr,RESPONSE_TO_SHOT_COMMAND))     			return;
		if(!sendStringUDP(socket_client,&opponent_sockaddr,readCoordinates)) 					return;
		if(!sendIntUDP(socket_client,&opponent_sockaddr,HIT))									return;
		if(my_ships_left == 0){
			printf("Era la tua ultima nave, hai perso! \n");
			my_turn = in_game = false;
			free(readCoordinates);
			return;

		}
	}else{
		printf("L'avversario ha colpito l'acqua \n");
		sendIntUDP(socket_client,&opponent_sockaddr,RESPONSE_TO_SHOT_COMMAND);
		sendStringUDP(socket_client,&opponent_sockaddr,readCoordinates);
		sendIntUDP(socket_client,&opponent_sockaddr,MISSED);

	}
	printf("\nE' il tuo turno, colpisci una casella avversaria \n");
	my_turn = true;
	timeout_main.tv_sec = TIMEOUT;
	free(readCoordinates);
}

void response_to_connection(){
	/*char* client_answer;
	int ret;
	
	scanf("%ms",&client_answer);
	


	if(!strcmp("y",client_answer)){
		ret = sendIntTCP(socket_server, MATCH_ACCEPTED);
		if(!ret)
			return;
		my_turn = false;
		have_to_response_to_connection = false;
		initialize_match();
		return;
	}
	else if(!strcmp("n",client_answer)){
		ret = sendIntTCP(socket_server,MATCH_REFUSED);
	if(!ret)
		return;
	}

	else{
		printf("Devi rispondere alla richiesta di connessione!\n\n");
		return;
	}*/
	char* client_answer;
	int i, ret,cmd, fdmax;
	fd_set read_fds,master_stdin;
	FD_ZERO(&read_fds);
	FD_ZERO(&master_stdin);
	
	FD_SET(0,&master_stdin);
	FD_SET(socket_server,&master_stdin);
	read_fds = master_stdin;									// 0 è stdin

	struct timeval timeout = {TIMEOUT,0}; 	
	fdmax = socket_server;	

	for(;;){
		//printf("ciao %d\n",(unsigned int) timeout.tv_sec);
		read_fds = master_stdin;	
		if(select(fdmax + 1, &read_fds, NULL, NULL,&timeout)<=0){
			printf("Timeout: troppo tardi per accettare la partita\n");
			have_to_response_to_connection = my_turn = in_game = opponent_ready = false;
			ret = sendIntTCP(socket_server,TIMEOUT_IN_CONNECTION);
			if(!ret)
				exit(1);
			//free(client_answer);
			return;
		}
		//printf("select risvegliata\n");
			
		for(i = 0; i <= fdmax + 1; i++){
			if(FD_ISSET(i,&read_fds)){
				if(i == 0){
					scanf("%ms",&client_answer);
					if(!strcmp("y",client_answer)){
						ret = sendIntTCP(socket_server, MATCH_ACCEPTED);
						if(!ret)
							return;
						my_turn = false;
						have_to_response_to_connection = false;
						initialize_match();
						free(client_answer);
						return;
					}
					else if(!strcmp("n",client_answer)){
						ret = sendIntTCP(socket_server,MATCH_REFUSED);
						my_turn = have_to_response_to_connection = false;

						if(!ret)
							return;
						free(client_answer);
						return;
					}
					//clean_buff();
					else{
						printf("Devi rispondere alla richiesta di connessione!\n\n");
						clean_buff();
						free(client_answer);
						//return;

					}

				}else if(i == socket_server){
					ret = recvIntTCP(socket_server,&cmd);
					if(!ret)
						return;
					if(cmd == CLIENT_DISCONNECTION){
						printf("Il tuo avversario si e' disconnesso durante l'invito\n");
						have_to_response_to_connection = in_game = my_turn = opponent_ready = false;
						return;
					}



				}
			}

		}

	}
}
	
void which_command_is_server(cmd){

	switch(cmd){
		case(OPPONENT_TIMEOUT_IN_GAME):
			printf("Hai vinto, il tuo avversario ha perso per TIMEOUT\n");
			in_game = my_turn = opponent_ready = have_to_response_to_connection = false;
			return;
		case(MATCH_REQUEST):
			//printf("qualcuno mi ha invitato\n");
			match_request();
			return;
		case(DISCONNECT_COMMAND):
			printf("Hai vinto, il tuo avversario si è ritirato dalla partita \n");
			in_game = my_turn = opponent_ready = false;
			return;
		case(CLIENT_DISCONNECTION):
			printf("Il tuo avversario si è disconnesso durante l'invito\n");
			have_to_response_to_connection = false;
			in_game = my_turn = opponent_ready = false;
			return;

	}	
	


}	

void which_command_is_input_in_game(){
	char* readCMD;
	scanf("%ms",&readCMD);

	if(strcmp(readCMD,"!shot")==0)
		shot_command();
	else if(strcmp(readCMD,"!help")==0)
		help_command_in_game();
	else if(strcmp(readCMD,"!show")==0)
		show_command();
	else if(strcmp(readCMD,"!disconnect") == 0)
		disconnect_command();
	else
		printf("Questo comando non esiste \n\n");
    clean_buff();
	free(readCMD);

		

}





			
void which_command_is_input_not_in_game(){
	char* readString;
	scanf("%ms",&readString);

	if(strcmp(readString,"!who")==0)
		who_command();
	else if(strcmp(readString,"!help")==0)
		help_command_not_in_game();
	else if(strcmp(readString,"!quit")==0)
		quit_command();
	else if(strcmp(readString,"!connect")==0)
		connect_command();
	else
		printf("Comando non riconosciuto\n\n");
	clean_buff();
	free(readString);

}

void which_command_is_client(int cmd){

	switch(cmd){
		case(SHOT_COMMAND):
			have_I_been_hit();
			return;
		case(RESPONSE_TO_SHOT_COMMAND):
			response_to_shot_command();
			return;
		case(I_AM_READY):
			opponent_ready = true;
			timeout_main.tv_sec = TIMEOUT;
			return;
	}
}

void which_command_is_input(){
	if(in_game)
		which_command_is_input_in_game();
	/*else if(have_to_response_to_connection)
		response_to_connection();*/
	else
		which_command_is_input_not_in_game();
}



int main(int argc, char *argv[]){
	int ret, fdmax,i;
	fd_set read_fds;
	struct sockaddr_in sv_addr;
	int  cmd;

	
	if(argc <3){
		printf("Errore, pochi parametri \n");
		return 0;
	}

	socket_server = socket (AF_INET,SOCK_STREAM,0);
	memset(&sv_addr,0,sizeof(sv_addr));
	sv_addr.sin_family=AF_INET;
	sv_addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET,argv[1],&sv_addr.sin_addr);
	ret=connect(socket_server, (struct sockaddr* ) &sv_addr, sizeof(sv_addr));
	
	if(ret){
		printf("Connessione al Server fallita \n");
		return 0;
	}

	printf("Connessione al Server: %s (porta %d) effettuata con successo \n \n", argv[1],atoi(argv[2]));
	help_command_not_in_game();
	ret = login();
	if(ret == LOGIN_FAIL){
	    printf("Si è verificato un errore durante il Login\n");
	    return 0;
	}
	printf("Login avvenuto con successo\n\n");

	ret = create_socket_client();
	if(ret != SOCKET_UDP_OK){
		//printf("Creazione socket UDP avvenuta con successo\n");

		printf("Creazione socket UDP fallita\n");	
		return 0;
	}

	FD_ZERO(&master);	
	FD_ZERO(&read_fds);

	FD_SET(socket_client,&master);
	FD_SET(socket_server,&master);
	FD_SET(0,&master);		// 0 è stdin

	//struct timeval timeout = {TIMEOUT,0};
	timeout_main.tv_sec = TIMEOUT;

	fdmax = (socket_server > socket_client)?socket_server:socket_client;

	for(;;){
		read_fds = master;
		if(select(fdmax + 1, &read_fds, NULL, NULL,&timeout_main)<=0){
			//printf("timeout scaduto?\n");
			timeout_main.tv_sec = TIMEOUT;
			if(in_game && my_turn && opponent_ready){
				printf("TIMEOUT: Hai perso\n");
				in_game = my_turn = opponent_ready = have_to_response_to_connection = false;
				ret = sendIntTCP(socket_server,TIMEOUT_IN_GAME);
				if(!ret) 
					return 0;

			}

		}
		for(i = 0; i <= fdmax+1; i++){
		    if(FD_ISSET(i,&read_fds)){
				if(i==0)
					which_command_is_input(); 
				else if(i == socket_server){
				//printf("mi sta contattando il server\n");
					ret = recvIntTCP(socket_server,&cmd);
					if(!ret) 
						return 0;
					which_command_is_server(cmd);
				}
				else if(i == socket_client){
					//printf("Richiesta dal client\n");
					recvIntUDP(socket_client,&opponent_sockaddr,&cmd);
					which_command_is_client(cmd);

				}

		    }
				
		}
	
	
	}

	return 0;
}

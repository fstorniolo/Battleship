#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#define SIDE 7
#define SHIPS 2
#define TIMEOUT 10


enum vars{LOGIN_OK,FREE,BUSY,CONNECTING,WHO_COMMAND,LOGIN_FAIL, SOCKET_UDP_OK, LOGIN_COMMAND,DISCONNECT_COMMAND,I_HAVE_WON,CLIENT_DISCONNECTION, QUIT_COMMAND,RESPONSE_TO_SHOT_COMMAND,HIT, MISSED,OPPONENT_TIMEOUT_IN_GAME,TIMEOUT_IN_CONNECTION, I_AM_READY,TIMEOUT_IN_GAME, CONNECT_COMMAND,SHOT_COMMAND, NO_USERNAME,MATCH_ACCEPTED, MATCH_REFUSED, USER_DISCONNECTED, MATCH_REQUEST, CONNECT_FAIL, WAITING_FOR_ANSWER};

int sendIntTCP(int,int);
int sendStringTCP(int,char*);
int recvIntTCP(int, int *);
char* recvStringTCP(int);
int sendStringUDP(int,struct sockaddr_in*,char*);
int sendIntUDP(int ,struct sockaddr_in *,int );
int recvIntUDP(int ,struct sockaddr_in *,int *);
char* recvStringUDP(int ,struct sockaddr_in *);



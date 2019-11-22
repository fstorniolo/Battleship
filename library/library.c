#include "library.h"


int sendIntTCP(int sd, int len){
	int ret;
	ret = send(sd, (void *) &len, sizeof(len),0);
	return (ret == sizeof(len));
}

int sendStringTCP(int sd,char* string){
	int len, ret;
	len = strlen(string)+1;
	//ret = send(sd, (void *) &len, sizeof(len),0);
	ret = sendIntTCP(sd,len);
	if(!ret){
		printf("Errore nella send");
		return 0;
		}
		
	ret = send(sd, (void *) string, len,0);
	return (ret == len);
}


int recvIntTCP(int sd, int *len){
	int ret, tmp;
	ret = recv(sd, (void *) &tmp, sizeof(tmp),0);
	*len = tmp;
	return (ret == sizeof(tmp));
}






char* recvStringTCP(int sd){
	int ret, len;
	char *buf;
	//ret = recv(sd, (void *)&len, sizeof(len),0);
	ret = recvIntTCP(sd, &len);
	if(!ret){
		printf("Errore durante la receive");
		return NULL;
	}
	
	buf = (char *)malloc(len);
	ret = recv(sd, (void *)buf, len,0);
	if(ret < len){
		printf("Errore durante la receive");
		return NULL;
	}
	return buf;
}


	
int sendStringUDP(int sd,struct sockaddr_in *sv_addr,char* buffer){
	int len,ret;//,addrlen;

	//addrlen = sizeof(cl_addr);

	len = strlen(buffer)+1;

	if(!sendIntUDP(sd,sv_addr,len))
		return 0;
	
	//ret = send(sd,buffer,len,0);
	ret = sendto(sd,buffer,len,0,(struct sockaddr*)sv_addr,sizeof(*sv_addr));
	if(ret < len){
		perror("[Errore] UDP send");
	} 
	
	return (len == ret);

}


int sendIntUDP(int sd,struct sockaddr_in *sv_addr,int value){
	int tosend;
	int ret;
	tosend = htonl(value);

	ret = sendto(sd,&tosend,sizeof(int),0,(struct sockaddr*)sv_addr,sizeof(*sv_addr));
	if(ret <= 0){
		perror("[Errore sendUDPInt]");
		return ret;
	}

	return (ret == sizeof(int));

}

int recvIntUDP(int sd,struct sockaddr_in *cl_addr,int *value){
	int torecv;
	int ret;
	socklen_t addrlen;

	addrlen = sizeof(cl_addr);

	ret = recvfrom(sd,&torecv,sizeof(int),0,(struct sockaddr*)cl_addr,&addrlen);
	if(ret <= 0){
		perror("[Errore recvIntUDP]");
		return ret;
	}

	*value = ntohl(torecv);

	return (ret == sizeof(int));

}

char* recvStringUDP(int sd,struct sockaddr_in *cl_addr){
	int ret,len;
	socklen_t addrlen;
    	addrlen = sizeof(*cl_addr);

	if(!recvIntUDP(sd,cl_addr,&len)){
		perror("[Errore] UDP recv");
		return NULL;
	}
	char *buffer = (char *)malloc(len);

	ret = recvfrom(sd,buffer,len,0,(struct sockaddr *)cl_addr,&addrlen);
	if(ret < len){
		perror("[Errore] recv");
		return NULL;	
	}
	return buffer;
}

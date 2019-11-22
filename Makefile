all: server.o client.o library.o
	gcc client.o library.o -o  client
	gcc server.o library.o -o  server

client.o: client.c
	gcc -c -g -Wall  client.c

server.o: server.c
	gcc -c -g -Wall  server.c
	
library.o: library/library.c library/library.h
	gcc -c -Wall  library/library.c

clean:
	rm -rf *.o 
	rm client
	rm server
	

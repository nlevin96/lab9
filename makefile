
all: client server

client: line_parser.o common.o client.o 
	gcc -g -Wall -o client line_parser.o common.o client.o

line_parser.o: line_parser.c
	gcc -g -Wall -c -o line_parser.o line_parser.c 
	
common.o: common.c
	gcc -g -Wall -c -o common.o common.c 
	
client.o: client.c
	gcc -g -Wall -c -o client.o client.c 


server:  server.o line_parser.o common.o
	gcc -g -Wall -o server common.o line_parser.o server.o
	
server.o: server.c
	gcc -g -Wall -c -o server.o server.c 
	

.PHONY: clean

clean: 
	rm -f *.o client server
 

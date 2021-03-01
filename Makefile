CC = gcc
CC_FLAGS = -w -g



all: test client server

server.o: server.cpp
	$(CXX) -Wall -c server.cpp -I.

client.o: client.cpp
	$(CXX) -Wall -c client.cpp -I.


main.o: main.cpp
	$(CXX) -Wall -I. -c main.cpp


test: main.o main_curses.o
	$(CXX) -L./ -Wall -lncurses  -o test main.o 


client: client.o
	$(CXX) -L./ -Wall -o cchat client.o

server: server.o
	$(CXX) -L./ -Wall -o cserverd server.o

clean:
	rm *.o *.a test server client

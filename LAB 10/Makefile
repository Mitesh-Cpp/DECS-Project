all: server client

server: multithreaded_server.cpp utilities.cpp declarations.h
	g++ multithreaded_server.cpp -o server -lpthread

client: client.cpp
	g++ client.cpp -o client -lpthread

clean:
	rm server client
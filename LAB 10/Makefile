all: server client client_test

server: multithreaded_server.cpp utilities.cpp declarations.h
	g++ multithreaded_server.cpp utilities.cpp -o server -lpthread

client: client.cpp
	g++ client.cpp -o client -lpthread

client_test: client_test.cpp
	g++ client_test.cpp -o client_test -lpthread

clean:
	rm server client client_test
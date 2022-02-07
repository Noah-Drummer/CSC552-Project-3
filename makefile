all: server client

server: server.cpp message.h
	g++ -fpermissive -o server server.cpp

client: client.cpp message.h
	g++ -o client client.cpp
	
clean:
	rm server client


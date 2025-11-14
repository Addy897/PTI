CFLAGS = -std=c++23 
LDFLAGS = -lws2_32
EXECUTABLE = main

all: client server

$(EXECUTABLE): $(EXECUTABLE).cpp
	g++ $(EXECUTABLE).cpp client.cpp server.cpp -o $(EXECUTABLE).exe $(CFLAGS) $(LDFLAGS)

message: message.cpp
	g++ message.cpp -DMESSAGE -o message.exe $(CFLAGS) $(LDFLAGS)
run: client
	 ./client.exe

server: server.cpp
	 g++ server.cpp -o server.exe $(CFLAGS) $(LDFLAGS)
pti_client: pti_client.cpp
	 g++ pti_client.cpp client.cpp mcp_server.cpp message.cpp -o pti_client.exe $(CFLAGS) $(LDFLAGS)
client: client.cpp
	 g++ client.cpp message.cpp -o client.exe $(CFLAGS) $(LDFLAGS)
signal_server: signal_server.cpp
	g++ signal_server.cpp message.cpp server.cpp -o signal_server.exe $(CFLAGS) $(LDFLAGS)

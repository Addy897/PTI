CFLAGS = -std=c++23  -Llib/
LDFLAGS = -lws2_32
EXECUTABLE = main

all: client server

$(EXECUTABLE): $(EXECUTABLE).cpp
	g++ $(EXECUTABLE).cpp client.cpp server.cpp -o $(EXECUTABLE).exe $(CFLAGS) $(LDFLAGS)

pti_client: pti_client.cpp
	 g++ -DPTI_CLI pti_client.cpp client.cpp mcp_server.cpp message.cpp -o pti_client.exe $(CFLAGS) $(LDFLAGS)

pti_gui: pti_gui.cpp
	g++ pti_gui.cpp pti_client.cpp client.cpp mcp_server.cpp message.cpp -o pti_gui.exe $(CFLAGS) $(LDFLAGS) -lraylib -lgdi32 -lwinmm
signal_server: signal_server.cpp
	g++ signal_server.cpp message.cpp server.cpp -o signal_server.exe $(CFLAGS) $(LDFLAGS)

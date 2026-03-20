CFLAGS = -std=c++23  -Llib/
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32 -lcrypto
    GUILDFLAGS =  -lraylib -lgdi32 -lwinmm
    EXEC_EXT = .exe
    RM = del /Q
else
    LDFLAGS = -lpthread -lcrypto
    GUILDFLAGS =  -lraylib -lX11
    EXEC_EXT =
    RM = rm -f
endif


EXECUTABLE = main

all: pti_gui

$(EXECUTABLE): $(EXECUTABLE).cpp
	g++ $(EXECUTABLE).cpp client.cpp server.cpp -o $(EXECUTABLE)$(EXEC_EXT) $(CFLAGS) $(LDFLAGS)

pti_client: pti_client.cpp
	 g++ -DPTI_CLI pti_client.cpp client.cpp mcp_server.cpp message.cpp -o pti_client$(EXEC_EXT) $(CFLAGS) $(LDFLAGS)

pti_gui: pti_gui.cpp pti_client.cpp client.cpp mcp_server.cpp message.cpp
	g++ pti_gui.cpp pti_client.cpp client.cpp mcp_server.cpp message.cpp -o pti_gui$(EXEC_EXT) $(CFLAGS) $(LDFLAGS) $(GUILDFLAGS) -ggdb 
signal_server: signal_server.cpp
	g++ signal_server.cpp message.cpp server.cpp -o signal_server$(EXEC_EXT) $(CFLAGS) $(LDFLAGS) -ggdb


clean: 
	rm $(EXECUTABLE)$(EXEC_EXT) pti_client$(EXEC_EXT) pti_gui$(EXEC_EXT) signal_server$(EXEC_EXT)

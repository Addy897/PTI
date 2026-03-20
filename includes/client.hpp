#pragma once
#include "message.hpp"
#ifdef _WIN32
#include "win.h"
#elif defined (__linux__)
#include <sys/socket.h>
typedef int SOCKET;
#endif

#include <cstdint>
#include <iostream>
#include <vector>
#define CHUNK_SIZE 1024
class Client {
public:
  Client();
  ~Client();
  void conn(std::string hostname, int port);
  void discon();
  int write(std::vector<uint8_t>);
  int read(size_t, uint8_t[], int flags = 0);
  Message readMessage();

protected:
#if win32
  WSAData wsdata;
#endif
  SOCKET client;
  sockaddr_in client_addr = {0};
};

#pragma once
#include "message.hpp"
#include "win.h"
#include <cstdint>
#include <iostream>
#include <vector>
#define CHUNK_SIZE 1024
class Client {
public:
  Client();
  ~Client();
  void conn(std::string hostname, int port);
  void close();
  int write(std::vector<uint8_t>);
  int read(size_t, uint8_t[], int flags = 0);
  Message readMessage();

protected:
  WSAData wsdata;
  SOCKET client;
  sockaddr_in client_addr = {0};
};

#pragma once
#include "server.hpp"

class SignalServer : public Server {
public:
  SignalServer(std::string hostname, int port) : Server(hostname, port) {}
  void handler(SOCKET) override;
};

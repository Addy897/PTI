#pragma once
#include "message.hpp"
#include "server.hpp"
#include <map>
#include <mutex>
#include <vector>
class SignalServer : public Server {
public:
  SignalServer(std::string hostname, int port) : Server(hostname, port) {}
  void handler(SOCKET) override;
  void handle_message(SOCKET client, Message &m);
  int write(SOCKET c, std::vector<uint8_t>);
  std::map<std::string, std::string> m_rooms;
  std::mutex m_room_mutex;
};

#pragma once
#include "client.hpp"
#include "mcp_server.hpp"
#include "message.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
class PTI {
private:
  Client m_client;
  std::unique_ptr<MCPServer> m_mcp_server;
  std::string m_server_ip;
  int m_server_port = 4444;
  int m_listen_port = 1234;
  std::map<std::string, bool> m_sessions;
  std::mutex m_sessions_mutex;
  std::map<std::string, std::string> m_computation_complete;

public:
  std::atomic<bool> m_running;

public:
  PTI();
  PTI(std::string server);
  PTI(std::string server, int port);
  void start();
  void clientHandler(std::string, std::string);
  void serverHandler(SOCKET);
  Message getMessage(SOCKET);
  std::string createROOM();
  std::string getRooms();
  void joinRoom(std::string);
};

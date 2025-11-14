#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <winsock2.h>

class MCPServer {
public:
  MCPServer();
  void start(bool non_block = true);
  ~MCPServer();
  void setHandler(std::function<void(SOCKET)>);
  std::function<void(SOCKET)> m_handler;
  std::atomic<bool> m_running;

private:
  std::string m_hostname = "";
  int m_port = 1234;

  WSAData m_wsdata;
  SOCKET m_server;
  sockaddr_in m_server_addr = {0};
};

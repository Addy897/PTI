#pragma once
#include <atomic>
#include <functional>
#include <string>
#include "win.h"
#define CHUNK_SIZE 1024
class Server {
public:
  Server(std::string hostname, int port);
  Server() {}
  void setHandler(std::function<void(SOCKET)>);
  ~Server();
  virtual void defaultHandler(SOCKET client);
  void start();
  void disconn();
  bool m_start_non_block = false;
  std::function<void(SOCKET)> m_handler;
  std::atomic<int> m_total_clients;
  std::atomic<bool> m_running;

protected:
  std::string m_hostname;
  int m_port;

  void show_clients();
#ifdef _WIN32
  WSAData m_wsdata;
#endif 
  SOCKET m_server;
  sockaddr_in m_server_addr = {0};
};

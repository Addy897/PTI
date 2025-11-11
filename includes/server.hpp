#pragma once
#include <atomic>
#include <string>
#include <winsock2.h>
#define CHUNK_SIZE 1024
class Server {
public:
  Server(std::string hostname, int port);
  ~Server();
  void start();
  virtual void handler(SOCKET client);
  void close();
  std::atomic<int> m_total_clients;

protected:
  std::string m_hostname;
  int m_port;

  void show_clients();
  WSAData m_wsdata;
  SOCKET m_server;
  sockaddr_in m_server_addr = {0};
};

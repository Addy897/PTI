#include "includes/mcp_server.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
MCPServer::MCPServer() {
  WORD version = MAKEWORD(2, 2);
  int ret = WSAStartup(version, &m_wsdata);
  if (ret) {
    char error[100];
    sprintf_s(error, "WSA initialization failed: %d.", ret);
    throw std::runtime_error(error);
  }
}

void MCPServer::setHandler(std::function<void(SOCKET)> handler) {
  m_handler = handler;
}
void MCPServer::start(bool non_block) {
  m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_server == INVALID_SOCKET) {
    throw std::runtime_error("socket init failed.");
  }
  u_long ip;
  if (m_hostname.empty()) {
    ip = INADDR_ANY;
  } else {
    ip = inet_addr(m_hostname.c_str());
  }

  m_server_addr.sin_family = AF_INET;
  m_server_addr.sin_port = htons(m_port);
  m_server_addr.sin_addr.s_addr = ip;

  int ret = bind(m_server, (SOCKADDR *)&m_server_addr, sizeof(m_server_addr));
  if (ret == SOCKET_ERROR) {
    ret = WSAGetLastError();
    std::cout << "Bind failed " << ret << "\n";
    return;
  }
  ret = listen(m_server, 5);
  if (ret == SOCKET_ERROR) {
    ret = WSAGetLastError();
    switch (ret) {
    case EADDRINUSE:
      throw std::runtime_error(
          "Another socket is already listening on the same port.");
    default: {
      char error[100];
      sprintf_s(error, "listen failed on port :%d ret :%d.", m_port, ret);

      throw std::runtime_error(error);
    }
    }
  }
  SOCKET client;
  std::vector<std::thread> threads;
  if (non_block) {

    u_long mode = 1;
    ioctlsocket(m_server, FIONBIO, &mode);
  }
  m_running.store(true);
  while (m_running.load()) {
    int client = accept(m_server, NULL, NULL);
    if (client == INVALID_SOCKET) {
      int err = WSAGetLastError();
      if (err != WSAEWOULDBLOCK) {
        printf("err : %d\n", err);
        perror("accept");
      }
      continue;
    }

    threads.emplace_back([this, client]() {
      if (this->m_handler)
        this->m_handler(client);
    });
  }
  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  if (m_server != INVALID_SOCKET) {
    closesocket(m_server);
    m_server = INVALID_SOCKET;
  }
}

MCPServer::~MCPServer() {
  if (m_server != INVALID_SOCKET) {
    closesocket(m_server);
    m_server = INVALID_SOCKET;
  }
  WSACleanup();
}

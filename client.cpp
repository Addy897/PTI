#include "includes/client.hpp"
#include <winerror.h>
#include <winsock2.h>
Client::Client() {
  WORD version = MAKEWORD(2, 2);
  int ret = WSAStartup(version, &wsdata);
  if (ret) {
    char error[100];
    sprintf_s(error, "WSA initialization failed: %d.", ret);
    throw std::runtime_error(error);
  }

  client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client == INVALID_SOCKET) {
    throw std::runtime_error("socket init failed.");
  }
}
void Client::conn(std::string addr, int port) {

  if (client == INVALID_SOCKET)
    throw std::runtime_error("invalid socket.");
  if (addr.empty()) {
    throw std::runtime_error("address is NULL");
  }

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = inet_addr(addr.c_str());
  int ret = connect(client, (SOCKADDR *)&client_addr, sizeof(client_addr));
  if (ret == SOCKET_ERROR) {

    throw std::runtime_error("connection failed.");
  }
}
int Client::write(std::vector<uint8_t> data) {
  if (client == INVALID_SOCKET)
    return SOCKET_ERROR;
  return send(client, (char *)data.data(), data.size(), 0);
}
int Client::read(size_t len, uint8_t buf[], int flags) {

  if (client == INVALID_SOCKET)
    return SOCKET_ERROR;
  return recv(client, (char *)buf, len, flags);
}
void Client::close() {
  if (client != INVALID_SOCKET) {
    closesocket(client);
    client = INVALID_SOCKET;
  }
}
Client::~Client() {
  close();
  WSACleanup();
}

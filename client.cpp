#include "client.hpp"
#include "message.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <synchapi.h>
#include <thread>
#include <vector>
#include <winerror.h>
#include <winsock2.h>

std::atomic<bool> running(true);
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

  if (addr.empty()) {
    throw std::runtime_error("hostname is NULL");
  }

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = inet_addr(addr.c_str());
  if (connect(client, (SOCKADDR *)&client_addr, sizeof(client_addr))) {

    throw std::runtime_error("connection failed.");
  }
}
int Client::write(std::vector<uint8_t> data) {
  return send(client, (char *)data.data(), data.size(), 0);
}
int Client::read(size_t len, uint8_t buf[]) {
  return recv(client, (char *)buf, len, 0);
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
void read_thread(Client &c) {
  while (running.load()) {
    uint8_t buf[CHUNK_SIZE];
    int read = c.read(CHUNK_SIZE, buf);
    if (read == 0) {
      running.store(false);
      printf("\nServer Closed\n");
    } else if (read < 0) {
      int ret = WSAGetLastError();
      if (ret == WSAECONNRESET) {
        running.store(false);
        printf("\nServer Closed\n");
        break;
      }
      printf("Read error: %d\n", ret);
    } else {
      Message m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});
      print_message(m);
    }
  }
}
int main() {
  static Client c;
  c.conn("127.0.0.1", 4444);
  std::string input;
  std::thread t([]() { read_thread(c); });
  while (running.load()) {
    std::cout << ">> ";
    std::cin >> input;
    if (input == "SEND") {
      std::cout << "Enter type: ";
      std::getline(std::cin, input);

      Message m(message_from_string(input));
      if (m.getType() == Message::MessageType::DATA ||
          m.getType() == Message::MessageType::JOIN_ROOM) {
        std::cout << "Enter data: ";
        std::cin >> input;
        m.setData(input);
      }
      if (m.getType() != Message::MessageType::EMPTY) {
        c.write(m.toBytes());
      }
    } else if (input == "q") {
      running.store(false);
    }
  }
  if (t.joinable())
    t.join();
  c.close();
}

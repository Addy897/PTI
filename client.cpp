#include "includes/client.hpp"
#include "includes/message.hpp"
#include <atomic>
#include <thread>
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
    throw std::runtime_error("address is NULL");
  }

  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = inet_addr(addr.c_str());
  int ret = connect(client, (SOCKADDR *)&client_addr, sizeof(client_addr));
  if (ret == SOCKET_ERROR) {

    throw std::runtime_error("connection failed.");
  }
  u_long mode = 1;
  ret = ioctlsocket(client, FIONBIO, &mode);
  if (ret != NO_ERROR) {
    printf("ioctlsocket failed with error: %d\n", ret);
    closesocket(client);
  }
}
int Client::write(std::vector<uint8_t> data) {
  return send(client, (char *)data.data(), data.size(), 0);
}
int Client::read(size_t len, uint8_t buf[], int flags) {
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
      } else if (ret == WSAEWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

      } else
        printf("Read error: %d\n", ret);
    } else {
      Message m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});
      printf("\n");
      print_message(m);
      printf("\n");
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
    std::getline(std::cin, input);
    if (input == "SEND") {
      std::cout << "Enter type: ";
      std::getline(std::cin, input);

      Message m(message_from_string(input));
      if (m.getType() == Message::MessageType::DATA ||
          m.getType() == Message::MessageType::JOIN_ROOM) {
        std::cout << "Enter data: ";
        std::getline(std::cin, input);
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

#include "includes/signal_server.hpp"
#include "includes/message.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
std::atomic<bool> running(true);
void SignalServer::handler(SOCKET client) {
  while (running.load()) {
    uint8_t buf[CHUNK_SIZE] = {0};
    int read = recv(client, (char *)buf, CHUNK_SIZE, 0);
    if (read == 0) {
      break;
    } else if (read > 0) {
      send(client, (char *)buf, read, 0);
      Message m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});
      print_message(m);
    }
  }
  closesocket(client);
}
void handle_cli(SignalServer &s) {
  std::string in;
  while (running.load()) {
    std::cout << ">> ";
    std::getline(std::cin, in);
    if (in == "CONN") {
      std::cout << "Total Clients: " << s.m_total_clients.load() << "\n";
    } else if (in == "q") {
      running.store(false);
    }
  }
}
int main() {
  static SignalServer s("127.0.0.1", 4444);
  std::thread t([]() { handle_cli(s); });
  s.start();
  atexit([]() { s.close(); });
  return 0;
}

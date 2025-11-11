#include "signal_server.hpp"
#include "message.hpp"
#include <vector>
void SignalServer::handler(SOCKET client) {
  {
    std::lock_guard<std::mutex> lock(m_total_clients_mutex);
    m_total_clients++;
  }
  while (true) {
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
  {
    std::lock_guard<std::mutex> lock(m_total_clients_mutex);
    m_total_clients--;
  }
}
int main() {
  static SignalServer s("127.0.0.1", 4444);
  s.start();
  atexit([]() { s.close(); });
  return 0;
}

#include "includes/signal_server.hpp"
#include "includes/message.hpp"
#include <atomic>
#include <iostream>
#include <mutex>
#include <psdk_inc/_socket_types.h>
#include <random>
#include <string>
#include <thread>
#include <winsock2.h>
std::atomic<bool> running(true);
void SignalServer::handle_message(SOCKET client, Message &m) {
  switch ((int)m.getType()) {
  case Message::CREATE_ROOM: {
    SOCKADDR_IN addr;
    int size = sizeof(addr);
    int read = getpeername(client, (SOCKADDR *)&addr, &size);
    if (read != SOCKET_ERROR) {
      char *c = inet_ntoa(addr.sin_addr);
      std::string id;
      std::random_device rd;
      std::mt19937 g(rd());
      std::uniform_int_distribution<int> dist(100000, 999999);
      {
        std::lock_guard<std::mutex> lg(m_room_mutex);
        do {
          id = std::to_string(dist(g));
        } while (m_rooms.contains(id));
        m_rooms[id] = c;
      }
      Message ms(Message::DATA);
      ms.setData(id);
      write(client, ms.toBytes());
    }
    break;
  }
  case Message::JOIN_ROOM: {
    if (m.getData().size() > 0) {
      std::string id = m.getDataAsString();
      Message ms(Message::DATA);
      {
        std::lock_guard<std::mutex> lg(m_room_mutex);
        if (m_rooms.contains(id)) {
          ms.setData(m_rooms[id]);
          m_rooms.erase(id);
        }
      }
      write(client, ms.toBytes());
    }
    break;
  }
  case Message::ROOMS: {

    std::string data;
    {
      std::lock_guard<std::mutex> g(m_room_mutex);
      for (auto &i : m_rooms) {
        data.append(i.first);
        data.append("\n");
      }
    }
    Message ms(Message::DATA);
    ms.setData(data);
    write(client, ms.toBytes());
    break;
  }
  }
}

int SignalServer::write(SOCKET c, std::vector<uint8_t> data) {
  if (c == INVALID_SOCKET)
    return SOCKET_ERROR;
  return send(c, (char *)data.data(), data.size(), 0);
}
void SignalServer::defaultHandler(SOCKET client) {
  while (running.load()) {
    Message m = Message::fromSocket(client);
    handle_message(client, m);
  }
  if (client != INVALID_SOCKET)
    closesocket(client);
}

void handle_cli(SignalServer &s) {
  std::string in;
  while (running.load()) {
    std::cout << ">> ";
    std::getline(std::cin, in);
    if (in == "CONN") {
      std::cout << "Total Clients: " << s.m_total_clients.load() << "\n";
    } else if (in == "ROOMS") {
      for (auto &it : s.m_rooms) {
        std::cout << it.first << ": " << it.second << "\n";
      }
    } else if (in == "q") {
      running.store(false);
    }
  }
}
int main() {
  static SignalServer s("", 4444);
  std::thread t([]() { handle_cli(s); });
  s.start();
  atexit([]() { s.close(); });
  return 0;
}

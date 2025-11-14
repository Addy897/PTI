#include "includes/pti_client.hpp"
#include "includes/message.hpp"
#include <psdk_inc/_socket_types.h>
#include <thread>
#include <winerror.h>
#include <winsock2.h>
PTI::PTI(std::string ip) {
  m_server_ip = ip;
  m_mcp_server = std::make_unique<MCPServer>();
}
PTI::PTI() { m_mcp_server = std::make_unique<MCPServer>(); }
PTI::PTI(std::string ip, int port) {
  m_server_ip = ip;
  m_server_port = port;
  m_mcp_server = std::make_unique<MCPServer>();
}
Message PTI::getMessage(SOCKET c) {
  uint8_t buf[CHUNK_SIZE];
  int read = recv(c, (char *)buf, CHUNK_SIZE, 0);

  if (read < 0) {
    int ret = WSAGetLastError();
    while (ret == WSAEWOULDBLOCK) {
      Sleep(1000);
      read = recv(c, (char *)buf, CHUNK_SIZE, 0);
      if (read > 0)
        break;
      ret = WSAGetLastError();
    }
  }
  if (read < 0) {
    Message m(Message::ERR);
    m.setData("Read error");
    return m;
  } else {

    Message m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});

    return m;
  }
}
Message PTI::getMessage(Client &c) {
  uint8_t buf[CHUNK_SIZE];
  int read = c.read(CHUNK_SIZE, buf, 0);

  if (read < 0) {
    int ret = WSAGetLastError();

    Message m(Message::ERR);
    if (ret == WSAECONNRESET) {
      m.setData("CONNECTION RESET");
    } else
      m.setData(std::to_string(ret));
    return m;
  } else {
    Message m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});
    return m;
  }
}

void PTI::clientHandler(std::string peer, std::string id) {

  Client c;
  const int PEER_PORT = 1234;
  c.conn(peer, 1234);
  Message m(Message::PEER_HLO);
  m.setData(id);
  c.write(m.toBytes());
  m = getMessage(c);
  if (m.getType() != Message::HLO_ACK) {
    printf("Recived unknown type %d instead of %d\n", m.getType(),
           Message::HLO_ACK);
    return;
  }
  m = Message(Message::DATA);
  m.setData("Hello There");
  c.write(m.toBytes());
  m = getMessage(c);
  print_message(m);
  c.write(m.toBytes());
}

void PTI::serverHandler(SOCKET c) {
  Message m = getMessage(c);
  if (m.getType() == Message::PEER_HLO) {
    std::string id = m.getDataAsString();

    {
      std::lock_guard<std::mutex> lg(m_sessions_mutex);
      if (m_sessions.contains(id) && m_sessions[id] == false) {
        m_sessions.erase(id);
      } else {
        closesocket(c);
        return;
      }
    }
    Message m1(Message::HLO_ACK);
    auto data = m1.toBytes();
    int ret = send(c, (char *)data.data(), data.size(), 0);

    if (ret == SOCKET_ERROR) {
      ret = WSAGetLastError();
      closesocket(c);
      return;
    }
    m = getMessage(c);
    data = m.toBytes();
    ret = send(c, (char *)data.data(), data.size(), 0);
    m = getMessage(c);
  }
  closesocket(c);
  {
    std::lock_guard<std::mutex> lg(m_sessions_mutex);
    if (m_sessions.size() == 0) {
      m_mcp_server->m_running.store(false);
    }
  }
}

Message PTI::getMessage() {
  uint8_t buf[CHUNK_SIZE];
  int read = m_client.read(CHUNK_SIZE, buf);
  Message m(Message::EMPTY);
  if (read == 0) {
    m_running.store(false);
  } else if (read < 0) {
    int ret = WSAGetLastError();
    if (ret == WSAECONNRESET) {
      m_running.store(false);
    } else
      printf("Read error: %d\n", ret);
  } else {
    m = Message::fromBytes(std::vector<uint8_t>{buf, buf + read});
  }
  return m;
}
std::string PTI::createROOM() {
  Message m(Message::CREATE_ROOM);
  m_client.write(m.toBytes());
  Message m1 = getMessage();
  std::string id = m1.getDataAsString();
  {
    std::lock_guard<std::mutex> lg(m_sessions_mutex);
    m_sessions[id] = false;
  }
  if (!m_mcp_server->m_running.load()) {
    m_mcp_server->setHandler([this](SOCKET c) { serverHandler(c); });
    std::thread t([this] { m_mcp_server->start(true); });
    t.detach();
  }
  return id;
}
std::string PTI::getRooms() {
  Message m(Message::ROOMS);
  m_client.write(m.toBytes());
  Message m1 = getMessage();
  return m1.getDataAsString();
}
void PTI::joinRoom(std::string id) {
  Message m(Message::JOIN_ROOM);
  m.setData(id);
  m_client.write(m.toBytes());
  Message m1 = getMessage();
  std::string peer = m1.getDataAsString();
  std::thread t([this, peer, id] { clientHandler(peer, id); });
  t.detach();
}
void PTI::start() {
  m_running.store(true);

  m_client.conn(m_server_ip, m_server_port);
}

int main() {
  PTI pti("127.0.0.1");
  pti.start();
  std::string input;
  while (pti.m_running.load()) {
    std::cout << ">> ";
    std::getline(std::cin, input);
    if (input == "q") {
      pti.m_running.store(false);
    }
    if (input == "ROOMS") {
      std::cout << "Rooms: " << pti.getRooms() << "\n";
    } else if (input == "CREATE_ROOM") {
      std::cout << "ID: " << pti.createROOM() << "\n";
    } else if (input == "JOIN_ROOM") {
      std::cout << "ID: ";
      std::string id;
      std::getline(std::cin, id);
      pti.joinRoom(id);
    }
  }
}

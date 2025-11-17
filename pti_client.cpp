#include "includes/pti_client.hpp"
#include "includes/message.hpp"
#include <thread>
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
Message PTI::getMessage(SOCKET c) { return Message::fromSocket(c); }

void PTI::clientHandler(std::string peer, std::string id) {

  Client c;
  const int PEER_PORT = 1234;
  c.conn(peer, 1234);
  // Handshake
  Message m(Message::PEER_HLO);
  m.setData(id);
  c.write(m.toBytes());
  m = c.readMessage();
  if (m.getType() != Message::HLO_ACK) {
    printf("Recived unknown type %d instead of %d\n", m.getType(),
           Message::HLO_ACK);
    return;
  }

  // MCP start
  // Send Single Encrypt A
  m = Message(Message::DATA);
  m.setData("Hello There");
  c.write(m.toBytes());

  // RECV Double Encrypt A & Single Encrypt B
  m = c.readMessage();
  print_message(m);

  // Send Double Encrypt B
  c.write(m.toBytes());

  // Push the data in the map;
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

std::string PTI::createROOM() {
  Message m(Message::CREATE_ROOM);
  m_client.write(m.toBytes());
  Message m1 = m_client.readMessage();
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
  Message m1 = m_client.readMessage();
  return m1.getDataAsString();
}
void PTI::joinRoom(std::string id) {
  Message m(Message::JOIN_ROOM);
  m.setData(id);
  m_client.write(m.toBytes());
  Message m1 = m_client.readMessage();
  std::string peer = m1.getDataAsString();

  std::thread t([this, peer, id] { clientHandler(peer, id); });
  t.detach();
}
void PTI::start() {
  m_running.store(true);

  m_client.conn(m_server_ip, m_server_port);
}
#ifdef PTI_CLI
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
#endif

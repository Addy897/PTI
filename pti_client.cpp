#include "includes/pti_client.hpp"
#include "includes/message.hpp"
#include <string>
#include <thread>

#include <fstream>
#include <functional> // for std::hash
#include <sstream>
#include <unordered_set>

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
    int ret = WSAGetLastError();
    m.setData("Read error:  " + std::to_string(ret));
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

// Simple salted hash for "privacy" (for real security, use SHA-256 lib)
std::string PTI::hashIndicator(const std::string &indicator) {
  static const std::string salt = "pti_demo_salt";
  std::hash<std::string> hasher;
  size_t h = hasher(indicator + salt);
  std::stringstream ss;
  ss << std::hex << h;
  return ss.str();
}

std::vector<std::string>
PTI::hashIndicators(const std::vector<std::string> &indicators) {
  std::vector<std::string> res;
  res.reserve(indicators.size());
  for (const auto &ind : indicators) {
    res.push_back(hashIndicator(ind));
  }
  return res;
}

std::string PTI::joinWithNewlines(const std::vector<std::string> &lines) {
  std::string out;
  for (size_t i = 0; i < lines.size(); ++i) {
    out += lines[i];
    if (i + 1 < lines.size())
      out += "\n";
  }
  return out;
}

std::vector<std::string> PTI::splitLines(const std::string &data) {
  std::vector<std::string> res;
  std::stringstream ss(data);
  std::string line;
  while (std::getline(ss, line)) {
    if (!line.empty())
      res.push_back(line);
  }
  return res;
}

void PTI::loadIndicatorsFromFile(const std::string &path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    std::cerr << "Failed to open indicators file: " << path << "\n";
    return;
  }
  m_indicators.clear();
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty())
      m_indicators.push_back(line);
  }
  std::cout << "Loaded " << m_indicators.size() << " indicators from " << path
            << "\n";
}

const std::vector<std::string> &PTI::getLastIntersection() const {
  return m_lastIntersection;
}

void PTI::clientHandler(std::string peer, std::string id) {
  Client c;
  const int PEER_PORT = 1234;
  c.conn(peer, PEER_PORT);

  // 1) Handshake: tell peer who we are
  Message m(Message::PEER_HLO);
  m.setData(id);
  c.write(m.toBytes());

  // 2) Wait for ACK
  m = getMessage(c);
  if (m.getType() != Message::HLO_ACK) {
    printf("Received unknown type %d instead of %d\n", m.getType(),
           Message::HLO_ACK);
    return;
  }

  // --- NEW: PSI PHASE (initiator) ---

  // Ensure we have indicators loaded
  if (m_indicators.empty()) {
    std::cout << "[PSI] Warning: no local indicators loaded.\n";
  }

  // Hash my indicators
  auto myHashes = hashIndicators(m_indicators);
  std::string myHashesStr = joinWithNewlines(myHashes);

  // 3) Send my hashes to peer
  Message psiMsg(Message::PSI_DATA);
  psiMsg.setData(myHashesStr);
  c.write(psiMsg.toBytes());

  // 4) Receive peer's hashes
  Message peerMsg = getMessage(c);
  if (peerMsg.getType() != Message::PSI_DATA) {
    std::cout << "[PSI] Expected PSI_DATA, got type " << peerMsg.getType()
              << "\n";
    return;
  }

  std::string peerHashesStr = peerMsg.getDataAsString();
  auto peerHashes = splitLines(peerHashesStr);

  // 5) Compute intersection: which of MY indicators are also in peer's set
  std::unordered_set<std::string> peerSet(peerHashes.begin(), peerHashes.end());
  m_lastIntersection.clear();

  for (size_t i = 0; i < m_indicators.size(); ++i) {
    if (i < myHashes.size() && peerSet.count(myHashes[i])) {
      m_lastIntersection.push_back(m_indicators[i]); // original indicator
    }
  }
}

void PTI::serverHandler(SOCKET c) {
  // 1) Read initial message (PEER_HLO)
  Message m = getMessage(c);
  if (m.getType() == Message::PEER_HLO) {
    std::string id = m.getDataAsString();

    {
      std::lock_guard<std::mutex> lg(m_sessions_mutex);
      if (m_sessions.contains(id) && m_sessions[id] == false) {
        // Mark session as active or remove, depending on your logic
        m_sessions.erase(id);
      } else {
        closesocket(c);
        return;
      }
    }

    // 2) Send HLO_ACK
    Message m1(Message::HLO_ACK);
    auto data = m1.toBytes();
    int ret = send(c, (char *)data.data(), data.size(), 0);
    if (ret == SOCKET_ERROR) {
      ret = WSAGetLastError();
      closesocket(c);
      return;
    }

    // --- NEW: PSI PHASE (responder) ---

    // 3) Receive peer's hashes
    Message peerMsg = getMessage(c);
    if (peerMsg.getType() != Message::PSI_DATA) {
      std::cout << "[PSI] Expected PSI_DATA, got type " << peerMsg.getType()
                << "\n";
      closesocket(c);
      return;
    }

    std::string peerHashesStr = peerMsg.getDataAsString();
    auto peerHashes = splitLines(peerHashesStr);
    std::unordered_set<std::string> peerSet(peerHashes.begin(),
                                            peerHashes.end());

    // Ensure we have indicators loaded
    if (m_indicators.empty()) {
      std::cout << "[PSI] Warning: no local indicators loaded (responder).\n";
    }

    // 4) Hash my indicators and send them
    auto myHashes = hashIndicators(m_indicators);
    std::string myHashesStr = joinWithNewlines(myHashes);
    Message myPsi(Message::PSI_DATA);
    myPsi.setData(myHashesStr);
    send(c, (char *)myPsi.toBytes().data(), myPsi.toBytes().size(), 0);

    // 5) Compute intersection locally (responder side too)
    m_lastIntersection.clear();
    for (size_t i = 0; i < m_indicators.size(); ++i) {
      if (i < myHashes.size() && peerSet.count(myHashes[i])) {
        m_lastIntersection.push_back(m_indicators[i]);
      }
    }
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
#ifdef PTI_CLI
int main() {
  PTI pti("127.0.0.1");
  pti.loadIndicatorsFromFile("indicators.txt");
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

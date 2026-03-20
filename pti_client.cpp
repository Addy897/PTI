#include "includes/pti_client.hpp"
#include "includes/message.hpp"
#include <string>
#include <thread>
#include <fstream>
#include <openssl/sha.h>
#include <sstream>
#include <unordered_set>
#include <iomanip>
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

std::string PTI::hashIndicator(const std::string &indicator) {
 unsigned char hash[32];
  std::string salted_indicator = indicator+"PTI_DEMO_SALT"; 
  SHA256((unsigned char *)salted_indicator.data(), salted_indicator.size(), hash);
std::stringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    
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
  out.reserve(lines.size()*33);
  for (size_t i = 0; i < lines.size(); ++i) {
    out += lines[i];
    if (i + 1 < lines.size())
      out += "\n";
  }
  return out;
}
static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n\0", 0, 6); 
    size_t end   = s.find_last_not_of(" \t\r\n\0", std::string::npos, 6);
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}
std::vector<std::string> PTI::splitLines(const std::string &data) {
    std::vector<std::string> res;
    size_t start = 0, end;
	while ((end = data.find('\n', start)) != std::string::npos) {
    res.push_back(trim(data.substr(start, end - start)));
    start = end + 1;
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
        line = trim(line);
	if (!line.empty()) {                              
           m_indicators.push_back(line);       
        }                                                 
    }

    std::unordered_set<std::string> seen;
    std::vector<std::string> deduped;
    for (const auto &ind : m_indicators) {
        if (seen.insert(ind).second) 
            deduped.push_back(ind);
    }
    m_indicators = std::move(deduped);
   std::cout << "Loaded " << m_indicators.size() << " unique indicators from " << path << "\n";

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
  m = c.readMessage();
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

  auto start_total = std::chrono::high_resolution_clock::now();


  auto start_hash = std::chrono::high_resolution_clock::now();
  auto myHashes = hashIndicators(m_indicators);
  auto end_hash = std::chrono::high_resolution_clock::now();


  auto start_joinLines = std::chrono::high_resolution_clock::now();
  std::string myHashesStr = joinWithNewlines(myHashes);
  auto end_joinLines = std::chrono::high_resolution_clock::now();





  auto start_transfer = std::chrono::high_resolution_clock::now();
  Message psiMsg(Message::PSI_DATA);
  psiMsg.setData(myHashesStr);
  c.write(psiMsg.toBytes());
  

  // 4) Receive peer's hashes
  Message peerMsg = c.readMessage();
  if (peerMsg.getType() != Message::PSI_DATA) {
    std::cout << "[PSI] Expected PSI_DATA, got type " << peerMsg.getType()
              << "\n";
    return;
  }

  auto end_transfer = std::chrono::high_resolution_clock::now();
  std::string peerHashesStr = peerMsg.getDataAsString();


  auto start_splitLines = std::chrono::high_resolution_clock::now();
  auto peerHashes = splitLines(peerHashesStr);
  auto end_splitLines = std::chrono::high_resolution_clock::now();


  auto start_psi = std::chrono::high_resolution_clock::now();
  // 5) Compute intersection: which of MY indicators are also in peer's set
  std::unordered_set<std::string> peerSet(peerHashes.begin(), peerHashes.end());
  m_lastIntersection.clear();

  for (size_t i = 0; i < m_indicators.size(); ++i) {
    if (i < myHashes.size() && peerSet.count(myHashes[i])) {
      m_lastIntersection.push_back(m_indicators[i]); // original indicator
    }
  }
  auto end_psi = std::chrono::high_resolution_clock::now();
  

  auto end_total = std::chrono::high_resolution_clock::now();

  // --- PRINT RESULTS TO CONSOLE ---
  auto dur_join = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_joinLines - start_joinLines)
                      .count();
  auto dur_split = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_splitLines - start_splitLines)
                      .count();
  auto dur_hash = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_hash - start_hash)
                      .count();
  auto dur_transfer = std::chrono::duration_cast<std::chrono::milliseconds>(
                          end_transfer - start_transfer)
                          .count();
  auto dur_psi =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_psi - start_psi)
          .count();
  auto dur_total = std::chrono::duration_cast<std::chrono::milliseconds>(
                       end_total - start_total)
                       .count();

  std::cout << "\n=== BENCHMARK RESULTS ===\n";
  std::cout << "Dataset Size: " << m_indicators.size() << "\n";
  std::cout << "Hashing Time: " << dur_hash << " ms\n";
  std::cout << "Join Time: " << dur_join << " ms\n";
  std::cout << "Split Time: " << dur_split << " ms\n";
  std::cout << "Transfer Time: " << dur_transfer << " ms\n";
  std::cout << "PSI Logic Time: " << dur_psi << " ms\n";
  std::cout << "Total Time: " << dur_total << " ms\n";
  std::cout << "=========================\n";
  // TODO: ADD Differential Privacy Logic
  if (m_result_handler)
    m_result_handler(m_lastIntersection, id, peer);
  m_lastIntersection.clear();
  m_indicators.clear();
}
void PTI::serverHandler(SOCKET c) {
  // 1) Read initial message (PEER_HLO)
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
    std::string peer = m_mcp_server->getPeer(c);
    if (m_result_handler)
      m_result_handler(m_lastIntersection, id, peer);

  }

  closesocket(c);
  {
    std::lock_guard<std::mutex> lg(m_sessions_mutex);
    if (m_sessions.size() == 0) {
      m_mcp_server->m_running.store(false);
    }
  }
}
void PTI::setResultHandler(
    std::function<void(const std::vector<std::string> &, const std::string &,
                       const std::string &)>
        handler) {
  m_result_handler = handler;
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
//#ifdef PTI_CLI
//int main() {
//  PTI pti("127.0.0.1");
//  pti.loadIndicatorsFromFile("indicators.txt");
//  pti.start();
//  std::string input;
//  while (pti.m_running.load()) {
//    std::cout << ">> ";
//    std::getline(std::cin, input);
//    if (input == "q") {
//      pti.m_running.store(false);
//    }
//    if (input == "ROOMS") {
//      std::cout << "Rooms: " << pti.getRooms() << "\n";
//    } else if (input == "CREATE_ROOM") {
//      std::cout << "ID: " << pti.createROOM() << "\n";
//    } else if (input == "JOIN_ROOM") {
//      std::cout << "ID: ";
//      std::string id;
//      std::getline(std::cin, id);
//      pti.joinRoom(id);
//    }
//  }
//}
//#endif

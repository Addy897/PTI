#pragma once
#include "client.hpp"
#include "mcp_server.hpp"
#include "message.hpp"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include <string>
#include <unordered_map>
#include <vector>

class PTI {
private:
  // existing fields:
  std::string m_server_ip;
  int m_server_port = 4444;
  Client m_client;
  std::unique_ptr<MCPServer> m_mcp_server;
  std::atomic_bool m_running{false};
  std::mutex m_sessions_mutex;
  std::unordered_map<std::string, bool> m_sessions;

  // --- NEW: PSI-related state ---
  std::vector<std::string> m_indicators;       // my local indicators
  std::vector<std::string> m_lastIntersection; // last PSI result

  // --- NEW: helper functions ---
  static std::string hashIndicator(const std::string &indicator);
  static std::vector<std::string>
  hashIndicators(const std::vector<std::string> &indicators);
  static std::string joinWithNewlines(const std::vector<std::string> &lines);
  static std::vector<std::string> splitLines(const std::string &data);

public:
  PTI();
  PTI(std::string server);
  PTI(std::string server, int port);
  void start();
  void clientHandler(std::string, std::string);
  void serverHandler(SOCKET);
  Message getMessage(SOCKET);
  std::string createROOM();
  std::string getRooms();
  void joinRoom(std::string);

  // --- NEW: load indicators and get last intersection ---
  void loadIndicatorsFromFile(const std::string &path);
  const std::vector<std::string> &getLastIntersection() const;
};

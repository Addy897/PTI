
#include "includes/mcp.hpp"
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

MCP::MCP() {
  std::random_device rd;
  std::mt19937 g(rd());
  std::uniform_int_distribution<uint8_t> dist(0, 255);
  m_key.resize(32);
  for (int i = 0; i < 32; i++)
    m_key[i] = dist(g);
}

void MCP::setA(const std::vector<std::string> &set) {
  m_setA = set;
  hashAndBlindA();
}

void MCP::hashAndBlindA() {
  m_blindedSetA.clear();
  for (const auto &s : m_setA) {
    std::vector<uint8_t> hash(32);
    SHA256(reinterpret_cast<const unsigned char *>(s.data()), s.size(),
           hash.data());
    for (size_t i = 0; i < 32; ++i)
      hash[i] ^= m_key[i];
    m_blindedSetA.push_back(hash);
  }
}
void MCP::setDoubleBlindedA(const std::vector<std::string> &doubleBlindedA) {
  m_blindedSetA.clear();
  for (const auto &hex : doubleBlindedA)
    m_blindedSetA.push_back(hexToBytes(hex));
}
void MCP::setBlindedB(const std::vector<std::string> &blindedB) {
  m_blindedSetB.clear();
  for (const auto &hex : blindedB)
    m_blindedSetB.push_back(hexToBytes(hex));
}

void MCP::blindB() {
  for (auto &v : m_blindedSetB)
    for (size_t i = 0; i < 32; ++i)
      v[i] ^= m_key[i];
}

std::vector<std::string> MCP::getBlindedA() const {
  std::vector<std::string> out;
  for (const auto &v : m_blindedSetA)
    out.push_back(bytesToHex(v));
  return out;
}
std::vector<std::string> MCP::getDoubleBlindedB() const {
  std::vector<std::string> out;
  for (const auto &v : m_blindedSetB)
    out.push_back(bytesToHex(v));
  return out;
}

std::vector<std::string> MCP::privateSetIntersection() const {
  std::vector<std::string> intersection;
  for (const auto &a : m_blindedSetA) {
    for (const auto &b : m_blindedSetB) {
      if (a == b)
        intersection.push_back(bytesToHex(a));
    }
  }
  for (const auto &a :)
    return intersection;
}

std::string MCP::bytesToHex(const std::vector<uint8_t> &data) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (uint8_t b : data)
    ss << std::setw(2) << static_cast<int>(b);
  return ss.str();
}

std::vector<uint8_t> MCP::hexToBytes(const std::string &hex) {
  if (hex.size() % 2 != 0)
    throw std::runtime_error("Invalid hex string");
  std::vector<uint8_t> bytes(hex.size() / 2);
  for (size_t i = 0; i < bytes.size(); ++i) {
    std::string byteStr = hex.substr(2 * i, 2);
    bytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
  }
  return bytes;
}

#ifndef MCP
int main() {
  MCP mcp_a;
  std::vector<std::string> in_a = {"TEST1", "TEST2", "TEST3"};
  MCP mcp_b;
  std::vector<std::string> in_b = {"TEST3", "TEST2", "TEST4", "TEST5", "TEST6"};
  mcp_a.setA(in_a);
  mcp_b.setA(in_b);

  auto blind_a = mcp_a.getBlindedA();
  auto blind_b = mcp_b.getBlindedA();

  mcp_a.setBlindedB(blind_b);
  mcp_b.setBlindedB(blind_a);

  blind_a = mcp_b.getDoubleBlindedB();
  blind_b = mcp_a.getDoubleBlindedB();

  mcp_a.setDoubleBlindedA(blind_a);
  mcp_b.setDoubleBlindedA(blind_b);
}

#endif

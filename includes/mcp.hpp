

#pragma once
#include <openssl/sha.h>
#include <string>
#include <vector>

class MCP {
public:
  MCP();
  ~MCP() = default;

  void setA(const std::vector<std::string> &set);

  void setBlindedB(const std::vector<std::string> &blindedB);
  void setDoubleBlindedA(const std::vector<std::string> &blindedA);

  void blindB();

  std::vector<std::string> getBlindedA() const;
  std::vector<std::string> getDoubleBlindedB() const;

  std::vector<std::string> privateSetIntersection() const;

private:
  std::vector<std::string> m_setA;
  std::vector<std::vector<uint8_t>> m_blindedSetA;
  std::vector<std::vector<uint8_t>> m_blindedSetB;
  std::vector<uint8_t> m_key;

  void hashAndBlindA();
  static std::string bytesToHex(const std::vector<uint8_t> &data);
  static std::vector<uint8_t> hexToBytes(const std::string &hex);
};

#pragma once
#include "win.h"
#include <cstdint>
#include <string>
#include <vector>
#define CHUNK_SIZE 1024
class Message {
public:
  enum MessageType {
    EMPTY = 0,
    ROOMS,
    DATA,
    CREATE_ROOM,
    JOIN_ROOM,
    EXIT_ROOM,
    ERR,
    PEER_HLO,
    HLO_ACK,
    PSI_DATA,
    PSI_RESULT
  };

private:
  std::vector<uint8_t> m_bytes = {0};
  std::vector<uint8_t> m_data = {0};
  MessageType m_type;

public:
  Message(MessageType type);

  static Message fromBytes(std::vector<uint8_t>);
  static Message fromSocket(SOCKET &);

  void setData(std::vector<uint8_t>);
  void setData(std::string);

  std::vector<uint8_t> toBytes();

  MessageType getType();
  std::vector<uint8_t> getData();
  std::string getDataAsString();
};
void print_message(Message &m);
Message::MessageType message_from_string(std::string);

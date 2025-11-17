#include "includes/message.hpp"
#include <map>
Message::Message(MessageType type) { m_type = type; }
void Message::setData(std::string data) {
  m_data.insert(m_data.begin(), data.begin(), data.end());
}
void Message::setData(std::vector<uint8_t> data) { m_data = data; }
std::vector<uint8_t> Message::toBytes() {
  std::vector<uint8_t> bytes;
  bytes.push_back(m_type);
  int size = m_data.size();
  bytes.push_back((size >> 24) & 0xFF);
  bytes.push_back((size >> 16) & 0xFF);
  bytes.push_back((size >> 8) & 0xFF);
  bytes.push_back(size & 0xFF);
  bytes.insert(bytes.end(), m_data.begin(), m_data.end());
  return bytes;
}

Message::MessageType Message::getType() { return m_type; }
std::vector<uint8_t> Message::getData() { return m_data; }
std::string Message::getDataAsString() {

  std::string s(m_data.begin(), m_data.end());
  if (s.ends_with('\0'))
    s.pop_back();
  return s;
}

Message Message::fromSocket(SOCKET &c) {
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
    MessageType type = static_cast<MessageType>(buf[0]);
    int size = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];

    Message m(type);
    if (size > 0)
      m.setData(std::vector<uint8_t>{buf + 5, buf + 5 + size});

    return m;
  }
}

Message Message::fromBytes(std::vector<uint8_t> data) {
  if (data.size() < 5)
    return Message(EMPTY);

  MessageType type = (MessageType)data[0];
  int size = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];

  Message m(type);

  if (data.size() >= 5 + size)
    m.setData(std::vector<uint8_t>(data.begin() + 5, data.begin() + 5 + size));

  return m;
}
void print_message(Message &m) {
  printf("===========================\n");
  printf("Type: ");
  switch ((int)m.getType()) {
  case Message::DATA:
    printf("DATA\n");
    break;
  case Message::CREATE_ROOM:
    printf("CREATE_ROOM\n");
    break;
  case Message::EXIT_ROOM:
    printf("EXIT_ROOM\n");
    break;
  case Message::ROOMS:
    printf("ROOMS\n");
    break;
  case Message::EMPTY:
    printf("EMTPY\n");
    break;
  case Message::PEER_HLO:
    printf("PEER_HLO\n");
    break;
  case Message::HLO_ACK:
    printf("HLO_ACK\n");
    break;
  }
  printf("Data: ");
  std::vector<uint8_t> bytes = m.getData();
  for (uint8_t byte : bytes) {
    printf("%x", byte);
  }
  printf("\n");
  printf("===========================\n");
}
Message::MessageType message_from_string(std::string type) {
  std::map<std::string, Message::MessageType> d = {
      {"DATA", Message::DATA},           {"ROOMS", Message::ROOMS},
      {"JOIN_ROOM", Message::JOIN_ROOM}, {"CREATE_ROOM", Message::CREATE_ROOM},
      {"EMTPY", Message::EMPTY},         {"EXIT_ROOM", Message::EXIT_ROOM},

  };
  return d[type];
}
#ifdef MESSAGE
int main() {
  Message m(Message::DATA);
  auto data = m.toBytes();
  Message m2 = Message::fromBytes(data);
  print_message(m2);

  return 0;
}
#endif

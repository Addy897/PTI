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
  bytes.push_back(m_data.size());
  bytes.insert(bytes.end(), m_data.begin(), m_data.end());
  return bytes;
}

Message::MessageType Message::getType() { return m_type; }
std::vector<uint8_t> Message::getData() { return m_data; }
std::string Message::getDataAsString() {

  return std::string{m_data.begin(), m_data.end()};
}
Message Message::fromBytes(std::vector<uint8_t> data) {
  if (data.size() < 1)
    return Message(EMPTY);
  MessageType type = static_cast<MessageType>(data[0]);
  Message m(type);
  if ((type == Message::DATA || type == Message::JOIN_ROOM) &&
      data.size() > 2) {

    m.setData(std::vector<uint8_t>{data.begin() + 2, data.end()});
  }
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

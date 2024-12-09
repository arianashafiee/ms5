#include "message.h"
#include <cassert>
#include <regex>
#include <map>

Message::Message()
  : m_message_type(MessageType::NONE)
{
}

Message::Message(MessageType message_type, std::initializer_list<std::string> args)
  : m_message_type(message_type),
    m_args(args)
{
}

Message::Message(const Message &other)
  : m_message_type(other.m_message_type),
    m_args(other.m_args)
{
}

Message::~Message()
{
}

Message &Message::operator=(const Message &rhs)
{
  if (this != &rhs) {
    m_message_type = rhs.m_message_type;
    m_args = rhs.m_args;
  }
  return *this;
}

MessageType Message::get_message_type() const
{
  return m_message_type;
}

void Message::set_message_type(MessageType message_type)
{
  m_message_type = message_type;
}

std::string Message::get_username() const
{
  if (m_message_type == MessageType::LOGIN && m_args.size() > 0) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_table() const
{
  if ((m_message_type == MessageType::CREATE ||
       m_message_type == MessageType::GET ||
       m_message_type == MessageType::SET) && m_args.size() > 0) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_key() const
{
  if ((m_message_type == MessageType::GET ||
       m_message_type == MessageType::SET) && m_args.size() > 1) {
    return m_args[1];
  }
  return "";
}

std::string Message::get_value() const {
  if (m_message_type == MessageType::PUSH || m_message_type == MessageType::DATA) {
    assert(m_args.size() >= 1);
    return m_args[0];
  }
  return "";
}

std::string Message::get_quoted_text() const
{
  if ((m_message_type == MessageType::FAILED ||
       m_message_type == MessageType::ERROR) && m_args.size() > 0) {
    return m_args[0];
  }
  return "";
}

void Message::push_arg(const std::string &arg)
{
  m_args.push_back(arg);
}

bool Message::is_valid() const
{
  if (m_message_type == MessageType::NONE) {
    return false;
  }

  // Define expected argument counts for each message type
  static const std::map<MessageType, std::pair<int,int>> arg_limits = {
    {MessageType::LOGIN, {1,1}},
    {MessageType::CREATE, {1,1}},
    {MessageType::PUSH, {1,1}},
    {MessageType::POP, {0,0}},
    {MessageType::TOP, {0,0}},
    {MessageType::SET, {2,2}},
    {MessageType::GET, {2,2}},
    {MessageType::ADD, {0,0}},
    {MessageType::SUB, {0,0}},
    {MessageType::MUL, {0,0}},
    {MessageType::DIV, {0,0}},
    {MessageType::BEGIN, {0,0}},
    {MessageType::COMMIT, {0,0}},
    {MessageType::BYE, {0,0}},
    {MessageType::OK, {0,0}},
    {MessageType::FAILED, {1,1}},
    {MessageType::ERROR, {1,1}},
    {MessageType::DATA, {1,1}}
  };

  auto it = arg_limits.find(m_message_type);
  if (it == arg_limits.end()) {
    // Unknown message type
    return false;
  }

  int num_args = (int)m_args.size();
  int min_args = it->second.first;
  int max_args = it->second.second;
  if (num_args < min_args || num_args > max_args) {
    return false;
  }

  // Validate identifiers where required
  // Identifiers: username, table, key must match ^[A-Za-z][A-Za-z0-9_]*$
  std::regex id_regex("^[A-Za-z][A-Za-z0-9_]*$");

  auto check_identifier = [&](const std::string &s) {
    return std::regex_match(s, id_regex);
  };

  // For LOGIN: username must be valid identifier
  if (m_message_type == MessageType::LOGIN) {
    if (!check_identifier(get_username())) return false;
  }

  // CREATE: table must be valid identifier
  if (m_message_type == MessageType::CREATE) {
    if (!check_identifier(get_table())) return false;
  }

  // GET: table and key must be valid identifiers
  if (m_message_type == MessageType::GET) {
    if (!check_identifier(get_table()) || !check_identifier(get_key())) return false;
  }

  // SET: table and key must be valid identifiers
  if (m_message_type == MessageType::SET) {
    if (!check_identifier(get_table()) || !check_identifier(get_key())) return false;
  }

  // Other message types either do not require identifier validation or have no args.

  return true;
}

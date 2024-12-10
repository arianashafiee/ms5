#include <set>
#include <map>
#include <regex>
#include <cassert>
#include "message.h"


Message::Message()
  : m_message_type(MessageType::NONE)
{
}

Message::Message( MessageType message_type, std::initializer_list<std::string> args )
  : m_message_type( message_type )
  , m_args( args )
{
}

Message::Message( const Message &other )
  : m_message_type( other.m_message_type )
  , m_args( other.m_args )
{
}

Message::~Message()
{
}

Message &Message::operator=( const Message &rhs )
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

void Message::push_arg( const std::string &arg )
{
  m_args.push_back( arg );
}

bool Message::is_valid() const
{
  // Define the minimum and maximum number of arguments for each message type
  static const std::map<MessageType, std::pair<int, int>> arg_limits = {
      {MessageType::LOGIN, {1, 1}},
      {MessageType::CREATE, {1, 1}},
      {MessageType::PUSH, {1, 1}},
      {MessageType::POP, {0, 0}},
      {MessageType::TOP, {0, 0}},
      {MessageType::SET, {2, 3}},  // SET requires table, key, and (optional) value
      {MessageType::GET, {2, 2}},
      {MessageType::ADD, {0, 0}},
      {MessageType::SUB, {0, 0}},
      {MessageType::MUL, {0, 0}},
      {MessageType::DIV, {0, 0}},
      {MessageType::BEGIN, {0, 0}},
      {MessageType::COMMIT, {0, 0}},
      {MessageType::BYE, {0, 0}},
      {MessageType::OK, {0, 0}},
      {MessageType::FAILED, {1, 1}},
      {MessageType::ERROR, {1, 1}},
      {MessageType::DATA, {1, 1}},
  };

  // Check for NONE type (invalid message)
  if (m_message_type == MessageType::NONE) {
    return false;
  }

  // Retrieve the expected argument count range for this message type
  auto it = arg_limits.find(m_message_type);
  if (it == arg_limits.end()) {
    return false; // Unknown message type
  }

  int num_args = m_args.size();
  int min_args = it->second.first;
  int max_args = it->second.second;

  // Validate the number of arguments
  if (num_args < min_args || num_args > max_args) {
    return false;
  }

  // Additional validation for specific message types
  switch (m_message_type) {
    case MessageType::LOGIN:
    case MessageType::CREATE:
    case MessageType::SET:
    case MessageType::GET:
      // Ensure table and key are valid identifiers (if present)
      for (const std::string &arg : m_args) {
        if (!std::regex_match(arg, std::regex("^[a-zA-Z][a-zA-Z0-9_]*$"))) {
          return false;
        }
      }
      break;

    case MessageType::FAILED:
    case MessageType::ERROR:
    case MessageType::DATA:
      // Ensure the argument (if present) does not contain invalid characters
      if (!m_args.empty() && m_args[0].find('\n') != std::string::npos) {
        return false;
      }
      break;

    default:
      // No additional checks for other message types
      break;
  }

  return true;
}

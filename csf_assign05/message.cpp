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

bool Message::is_valid() const {
  switch (m_message_type) {
    case MessageType::LOGIN:
      return m_args.size() == 1 && is_identifier(m_args[0]);
    case MessageType::CREATE:
      return m_args.size() == 1 && is_identifier(m_args[0]);
    case MessageType::PUSH:
      return m_args.size() == 1 && is_value(m_args[0]);
    case MessageType::SET:
      return m_args.size() == 2 && is_identifier(m_args[0]) && is_identifier(m_args[1]);
    case MessageType::GET:
      return m_args.size() == 2 && is_identifier(m_args[0]) && is_identifier(m_args[1]);
    case MessageType::BYE:
      return m_args.empty();
    case MessageType::DATA:
      return m_args.size() == 1 && is_value(m_args[0]);
    default:
      return false;
  }
}


// Validates if a string is an identifier
bool is_identifier(const std::string &str) {
    // An identifier starts with a letter and is followed by letters, digits, or underscores
    static const std::regex identifier_regex("^[a-zA-Z][a-zA-Z0-9_]*$");
    return std::regex_match(str, identifier_regex);
}

// Validates if a string is a value
bool is_value(const std::string &str) {
    // A value must be non-empty and contain no whitespace
    return !str.empty() && str.find_first_of(" \t\n") == std::string::npos;
}

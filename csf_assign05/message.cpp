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
  if (m_message_type == MessageType::NONE) return false;

  switch (m_message_type) {
    case MessageType::LOGIN:
      return m_args.size() == 1;
    case MessageType::CREATE:
      return m_args.size() == 1;
    case MessageType::PUSH:
      return m_args.size() == 1;
    case MessageType::SET:
    case MessageType::GET:
      return m_args.size() == 2;
    case MessageType::ADD:
    case MessageType::SUB:
    case MessageType::MUL:
    case MessageType::DIV:
    case MessageType::POP:
    case MessageType::TOP:
    case MessageType::BEGIN:
    case MessageType::COMMIT:
    case MessageType::BYE:
      return m_args.empty();
    case MessageType::OK:
    case MessageType::FAILED:
    case MessageType::ERROR:
    case MessageType::DATA:
      return !m_args.empty();
    default:
      return false;
  }
}

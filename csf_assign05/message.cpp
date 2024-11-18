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

std::string Message::get_value() const
{
  if (m_message_type == MessageType::SET && m_args.size() > 2) {
    return m_args[2];
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
  // Validate based on the message type and its arguments
  switch (m_message_type) {
    case MessageType::LOGIN:
      return m_args.size() == 1; // LOGIN requires one argument (username)
    case MessageType::CREATE:
      return m_args.size() == 1; // CREATE requires one argument (table name)
    case MessageType::GET:
      return m_args.size() == 2; // GET requires table and key
    case MessageType::SET:
      return m_args.size() == 3; // SET requires table, key, and value
    case MessageType::PUSH:
    case MessageType::DATA:
      return m_args.size() == 1; // PUSH/DATA require one argument (value)
    case MessageType::FAILED:
    case MessageType::ERROR:
      return m_args.size() == 1; // FAILED/ERROR require one quoted_text
    case MessageType::OK:
    case MessageType::POP:
    case MessageType::TOP:
    case MessageType::ADD:
    case MessageType::SUB:
    case MessageType::MUL:
    case MessageType::DIV:
    case MessageType::BEGIN:
    case MessageType::COMMIT:
    case MessageType::BYE:
      return m_args.empty(); // These message types don't take any arguments
    default:
      return false; // Invalid message type
  }
}

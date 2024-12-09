#include "client_connection.h"
#include "server.h"
#include "exceptions.h"
#include "table.h"
#include "message_serialization.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <regex>

ClientConnection::ClientConnection(Server *server, int client_fd)
  : m_server(server), m_client_fd(client_fd), m_inTransaction(false)
{
  rio_readinitb(&m_fdbuf, m_client_fd);
}

ClientConnection::~ClientConnection()
{
  Close(m_client_fd);
}

// Since is_valid() handles identifier checks, this may not be needed for message validity.
// It's kept here in case needed for arithmetic checks, etc.
bool ClientConnection::is_valid_identifier(const std::string &s) const {
  std::regex r("^[A-Za-z][A-Za-z0-9_]*$");
  return std::regex_match(s, r);
}

bool ClientConnection::is_integer(const std::string &s) const {
  if (s.empty()) return false;
  size_t start = 0;
  if (s[0] == '-') start = 1;
  for (size_t i=start; i<s.size(); i++) {
    if (!isdigit((unsigned char)s[i])) return false;
  }
  return true;
}

void ClientConnection::send_ok() {
  send_response(MessageType::OK);
}

void ClientConnection::send_failed(const std::string &reason) {
  send_response(MessageType::FAILED, reason);
}

void ClientConnection::send_error(const std::string &reason) {
  send_response(MessageType::ERROR, reason);
}

void ClientConnection::send_data(const std::string &value) {
  Message msg(MessageType::DATA, {value});
  std::string encoded;
  MessageSerialization::encode(msg, encoded);
  Rio_writen(m_client_fd, encoded.c_str(), encoded.size());
}

void ClientConnection::send_response(MessageType type, const std::string &arg) {
  Message msg(type);
  if (!arg.empty()) {
    msg.push_arg(arg);
  }
  std::string encoded;
  MessageSerialization::encode(msg, encoded);
  Rio_writen(m_client_fd, encoded.c_str(), encoded.size());
}

void ClientConnection::lock_table_autocommit(Table *tbl) {
  tbl->lock();
}

void ClientConnection::lock_table_transaction(Table *tbl) {
  if (m_lockedTables.count(tbl)) return; // already locked

  if (!tbl->trylock()) {
    throw FailedTransaction("Failed to acquire table lock for transaction");
  }
  m_lockedTables.insert(tbl);
}

void ClientConnection::commit_transaction() {
  for (auto tbl : m_lockedTables) {
    tbl->commit_changes();
    tbl->unlock();
  }
  m_lockedTables.clear();
  m_inTransaction = false;
}

void ClientConnection::rollback_transaction() {
  for (auto tbl : m_lockedTables) {
    tbl->rollback_changes();
    tbl->unlock();
  }
  m_lockedTables.clear();
  m_inTransaction = false;
}

void ClientConnection::chat_with_client()
{
  bool done = false;
  bool logged_in = false;

  try {
    while (!done) {
      char buffer[Message::MAX_ENCODED_LEN];
      ssize_t rc = rio_readlineb(&m_fdbuf, buffer, Message::MAX_ENCODED_LEN);
      if (rc == 0) {
        // Client closed connection
        done = true;
        break;
      }
      std::string line(buffer);

      Message request;
      try {
        MessageSerialization::decode(line, request);

        // Check message validity here
        // If invalid, decode() would have thrown InvalidMessage already
        // or we can check again:
        if (!request.is_valid()) {
          throw InvalidMessage("Invalid message");
        }

        MessageType t = request.get_message_type();
        if (!logged_in && t != MessageType::LOGIN) {
          // First message must be LOGIN
          throw InvalidMessage("First message must be LOGIN");
        }

        switch(t) {
          case MessageType::LOGIN:
            handle_LOGIN(request);
            logged_in = true;
            break;
          case MessageType::CREATE:
            handle_CREATE(request);
            break;
          case MessageType::PUSH:
            handle_PUSH(request);
            break;
          case MessageType::POP:
            handle_POP(request);
            break;
          case MessageType::TOP:
            handle_TOP(request);
            break;
          case MessageType::SET:
            handle_SET(request);
            break;
          case MessageType::GET:
            handle_GET(request);
            break;
          case MessageType::ADD:
            handle_ADD(request);
            break;
          case MessageType::SUB:
            handle_SUB(request);
            break;
          case MessageType::MUL:
            handle_MUL(request);
            break;
          case MessageType::DIV:
            handle_DIV(request);
            break;
          case MessageType::BEGIN:
            handle_BEGIN(request);
            break;
          case MessageType::COMMIT:
            handle_COMMIT(request);
            break;
          case MessageType::BYE:
            handle_BYE(request);
            done = true;
            break;
          default:
            // If we got here, message type is not recognized
            throw InvalidMessage("Unknown request type");
        }

      } catch (InvalidMessage &imex) {
        // Invalid message → ERROR and end connection
        send_error(imex.what());
        done = true;
      } catch (OperationException &opex) {
        // Operation failed, but not invalid message
        // FAILED response, rollback if transaction
        if (m_inTransaction) {
          rollback_transaction();
        }
        send_failed(opex.what());
      } catch (FailedTransaction &ftex) {
        // Transaction-specific failure
        rollback_transaction();
        send_failed(ftex.what());
      }
    }
  } catch (CommException &cex) {
    // Communication error, end silently
  }

  if (m_inTransaction) {
    rollback_transaction();
  }
}

void ClientConnection::handle_LOGIN(const Message &msg) {
  // Already validated by is_valid()
  send_ok();
}

void ClientConnection::handle_CREATE(const Message &msg) {
  // Table name validity was checked by is_valid(), so we just create it
  std::string tableName = msg.get_table();
  m_server->lock_tables_map();
  try {
    Table *t = m_server->find_table(tableName);
    if (t != nullptr) {
      m_server->unlock_tables_map();
      throw OperationException("Table already exists");
    }
    m_server->create_table(tableName);
    m_server->unlock_tables_map();
  } catch(...) {
    m_server->unlock_tables_map();
    throw;
  }
  send_ok();
}

void ClientConnection::handle_PUSH(const Message &msg) {
  // Pushing value onto stack
  std::string val = msg.get_value();
  // is_valid() ensures we had 1 argument and not empty
  m_stack.push(val);
  send_ok();
}

void ClientConnection::handle_POP(const Message &msg) {
  // POP top of stack
  (void)msg;
  if (m_stack.is_empty()) {
    throw OperationException("Stack empty, cannot pop");
  }
  m_stack.pop();
  send_ok();
}

void ClientConnection::handle_TOP(const Message &msg) {
  (void)msg;
  if (m_stack.is_empty()) {
    throw OperationException("Stack empty, cannot top");
  }
  std::string top_val = m_stack.get_top();
  send_data(top_val);
}

void ClientConnection::handle_SET(const Message &msg) {
  std::string tableName = msg.get_table();
  std::string key = msg.get_key();
  if (m_stack.is_empty()) {
    throw OperationException("No value on stack to SET");
  }
  std::string value = m_stack.get_top();
  m_stack.pop();

  m_server->lock_tables_map();
  Table *tbl = m_server->find_table(tableName);
  m_server->unlock_tables_map();
  if (!tbl) {
    throw OperationException("No such table");
  }

  if (m_inTransaction) {
    lock_table_transaction(tbl);
    tbl->set(key, value);
    // don't commit yet, wait for COMMIT
  } else {
    lock_table_autocommit(tbl);
    tbl->set(key, value);
    tbl->commit_changes();
    tbl->unlock();
  }

  send_ok();
}

void ClientConnection::handle_GET(const Message &msg) {
  std::string tableName = msg.get_table();
  std::string key = msg.get_key();

  m_server->lock_tables_map();
  Table *tbl = m_server->find_table(tableName);
  m_server->unlock_tables_map();

  if (!tbl) {
    throw OperationException("No such table");
  }

  std::string val;
  if (m_inTransaction) {
    lock_table_transaction(tbl);
    val = tbl->get(key);
  } else {
    lock_table_autocommit(tbl);
    val = tbl->get(key);
    tbl->unlock();
  }

  m_stack.push(val);
  send_ok();
}

void ClientConnection::handle_ADD(const Message &msg) {
  (void)msg;
  if (m_stack.is_empty()) throw OperationException("Not enough operands for ADD");
  std::string v1 = m_stack.get_top();
  m_stack.pop();
  if (m_stack.is_empty()) {
    m_stack.push(v1);
    throw OperationException("Not enough operands for ADD");
  }
  std::string v2 = m_stack.get_top();
  m_stack.pop();

  if (!is_integer(v1) || !is_integer(v2)) {
    throw OperationException("Non-integer operand for ADD");
  }

  int i1 = std::stoi(v1);
  int i2 = std::stoi(v2);
  m_stack.push(std::to_string(i1 + i2));
  send_ok();
}

void ClientConnection::handle_SUB(const Message &msg) {
  (void)msg;
  if (m_stack.is_empty()) throw OperationException("Not enough operands for SUB");
  std::string rightVal = m_stack.get_top();
  m_stack.pop();
  if (m_stack.is_empty()) {
    m_stack.push(rightVal);
    throw OperationException("Not enough operands for SUB");
  }
  std::string leftVal = m_stack.get_top();
  m_stack.pop();

  if (!is_integer(leftVal) || !is_integer(rightVal)) {
    throw OperationException("Non-integer operand for SUB");
  }

  int leftInt = std::stoi(leftVal);
  int rightInt = std::stoi(rightVal);
  m_stack.push(std::to_string(leftInt - rightInt));
  send_ok();
}

void ClientConnection::handle_MUL(const Message &msg) {
  (void)msg;
  if (m_stack.is_empty()) throw OperationException("Not enough operands for MUL");
  std::string v1 = m_stack.get_top();
  m_stack.pop();
  if (m_stack.is_empty()) {
    m_stack.push(v1);
    throw OperationException("Not enough operands for MUL");
  }
  std::string v2 = m_stack.get_top();
  m_stack.pop();

  if (!is_integer(v1) || !is_integer(v2)) {
    throw OperationException("Non-integer operand for MUL");
  }

  long i1 = std::stol(v1);
  long i2 = std::stol(v2);
  m_stack.push(std::to_string(i1 * i2));
  send_ok();
}

void ClientConnection::handle_DIV(const Message &msg) {
  (void)msg;
  if (m_stack.is_empty()) throw OperationException("Not enough operands for DIV");
  std::string rightVal = m_stack.get_top();
  m_stack.pop();
  if (m_stack.is_empty()) {
    m_stack.push(rightVal);
    throw OperationException("Not enough operands for DIV");
  }
  std::string leftVal = m_stack.get_top();
  m_stack.pop();

  if (!is_integer(leftVal) || !is_integer(rightVal)) {
    throw OperationException("Non-integer operand for DIV");
  }

  int leftInt = std::stoi(leftVal);
  int rightInt = std::stoi(rightVal);
  if (rightInt == 0) {
    throw OperationException("Division by zero");
  }
  m_stack.push(std::to_string(leftInt / rightInt));
  send_ok();
}

void ClientConnection::handle_BEGIN(const Message &msg) {
  (void)msg;
  if (m_inTransaction) {
    // Nested transactions not allowed
    throw FailedTransaction("Nested transactions not allowed");
  }
  m_inTransaction = true;
  send_ok();
}

void ClientConnection::handle_COMMIT(const Message &msg) {
  (void)msg;
  if (!m_inTransaction) {
    throw OperationException("No transaction in progress");
  }
  commit_transaction();
  send_ok();
}

void ClientConnection::handle_BYE(const Message &msg) {
  (void)msg;
  send_ok();
}

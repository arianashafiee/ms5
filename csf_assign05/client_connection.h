#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <set>
#include <map>
#include <string>
#include "message.h"
#include "csapp.h"
#include "value_stack.h"

class Server; // forward declaration
class Table;  // forward declaration

class ClientConnection {
private:
  Server *m_server;
  int m_client_fd;
  rio_t m_fdbuf;

  // Operand stack
  ValueStack m_stack;

  // Transaction state
  bool m_inTransaction;
  std::set<Table*> m_lockedTables;

  // copy constructor and assignment operator are prohibited
  ClientConnection( const ClientConnection & );
  ClientConnection &operator=( const ClientConnection & );

  // Helper methods for handling requests
  void handle_LOGIN(const Message &msg);
  void handle_CREATE(const Message &msg);
  void handle_PUSH(const Message &msg);
  void handle_POP(const Message &msg);
  void handle_TOP(const Message &msg);
  void handle_SET(const Message &msg);
  void handle_GET(const Message &msg);
  void handle_ADD(const Message &msg);
  void handle_SUB(const Message &msg);
  void handle_MUL(const Message &msg);
  void handle_DIV(const Message &msg);
  void handle_BEGIN(const Message &msg);
  void handle_COMMIT(const Message &msg);
  void handle_BYE(const Message &msg);

  // Utility methods
  void send_response(MessageType type, const std::string &arg = "");
  void send_ok();
  void send_failed(const std::string &reason);
  void send_error(const std::string &reason);
  void send_data(const std::string &value);

  // Locking helpers
  // Acquire lock for a single table in autocommit mode
  void lock_table_autocommit(Table *tbl);
  // Try to lock a table in transaction mode
  void lock_table_transaction(Table *tbl);

  // Commit/rollback utilities
  void commit_transaction();
  void rollback_transaction();

  // Validate identifiers and integers
  bool is_valid_identifier(const std::string &s) const;
  bool is_integer(const std::string &s) const;

public:
  ClientConnection( Server *server, int client_fd );
  ~ClientConnection();

  void chat_with_client();
};

#endif // CLIENT_CONNECTION_H

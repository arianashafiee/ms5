#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <string>
#include <memory>
#include <pthread.h>
#include "table.h"
#include "client_connection.h"

class Server {
private:
  int m_listen_fd; // File descriptor for the listening socket
  std::map<std::string, std::unique_ptr<Table>> m_tables; // Map of table names to Table objects
  pthread_mutex_t m_table_lock; // Mutex for synchronizing access to m_tables

  // Copy constructor and assignment operator are prohibited
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

public:
  Server();
  ~Server();

  void listen(const std::string &port); // Start listening for client connections
  void server_loop(); // Main server loop for accepting and handling connections

  static void *client_worker(void *arg); // Worker thread function for client handling

  void log_error(const std::string &what); // Log error messages

  // Member functions for managing tables
  void create_table(const std::string &name); // Create a new table
  Table *find_table(const std::string &name); // Find an existing table by name
};

#endif // SERVER_H

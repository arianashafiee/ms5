#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <string>
#include <pthread.h>
#include "table.h"

class ClientConnection; // forward declaration

class Server {
private:
  int m_listenfd; // Listening socket file descriptor
  pthread_mutex_t m_tables_mutex; // Mutex for m_tables map
  std::map<std::string, Table*> m_tables;

  // copy constructor and assignment operator are prohibited
  Server( const Server & );
  Server &operator=( const Server & );

public:
  Server();
  ~Server();

  // Start listening on the specified port
  void listen(const std::string &port);

  // Accept connections and create a thread per client
  void server_loop();

  static void *client_worker(void *arg);

  static void log_error(const std::string &what);

  // Create a table with the given name. Throws OperationException if exists.
  void create_table(const std::string &name);

  // Find a table by name (caller must hold m_tables_mutex)
  Table *find_table(const std::string &name);

  void lock_tables_map() { pthread_mutex_lock(&m_tables_mutex); }
  void unlock_tables_map() { pthread_mutex_unlock(&m_tables_mutex); }
};

#endif // SERVER_H

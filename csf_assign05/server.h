#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <string>
#include <pthread.h>
#include "table.h"
#include "client_connection.h"

class Server {
private:
  int m_listenfd; // file descriptor for the listening socket
  pthread_mutex_t m_tables_mutex;
  std::map<std::string, Table*> m_tables;

  // copy constructor and assignment operator are prohibited
  Server( const Server & );
  Server &operator=( const Server & );

public:
  Server();
  ~Server();

  // Start listening on the specified port
  void listen( const std::string &port );

  // Accept connections in a loop, create client threads
  void server_loop();

  static void *client_worker( void *arg );

  void log_error( const std::string &what );

  // Create a table with the given name. Throws OperationException if table exists or invalid name.
  void create_table( const std::string &name );

  // Find a table by name. Returns pointer or nullptr if not found.
  // Caller must hold the m_tables_mutex lock when calling this.
  Table *find_table( const std::string &name );

  // Lock/unlock the global map of tables
  void lock_tables_map() { pthread_mutex_lock(&m_tables_mutex); }
  void unlock_tables_map() { pthread_mutex_unlock(&m_tables_mutex); }

};

#endif // SERVER_H

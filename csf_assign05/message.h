#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <string>
#include <pthread.h>
#include "table.h"
#include "client_connection.h"

class Server {
private:
  int m_listenfd;                      // listening socket file descriptor
  pthread_mutex_t m_tables_mutex;      // mutex for accessing m_tables
  std::map<std::string, Table*> m_tables;

  // copy constructor and assignment operator are prohibited
  Server(const Server &);
  Server &operator=(const Server &);

public:
  Server();
  ~Server();

  void listen(const std::string &port);
  void server_loop();

  static void *client_worker(void *arg);

  void log_error(const std::string &what);

  void create_table(const std::string &name);
  Table *find_table(const std::string &name);

  void lock_tables_map() { pthread_mutex_lock(&m_tables_mutex); }
  void unlock_tables_map() { pthread_mutex_unlock(&m_tables_mutex); }
};

#endif // SERVER_H

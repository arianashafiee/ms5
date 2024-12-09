#include <iostream>
#include <cassert>
#include <memory>
#include <sstream>
#include "csapp.h"
#include "exceptions.h"
#include "guard.h"
#include "server.h"
#include "client_connection.h"

Server::Server()
  : m_listenfd(-1)
{
  pthread_mutex_init(&m_tables_mutex, nullptr);
}

Server::~Server()
{
  // Clean up allocated tables
  {
    Guard g(m_tables_mutex);
    for (auto &entry : m_tables) {
      delete entry.second;
    }
    m_tables.clear();
  }
  pthread_mutex_destroy(&m_tables_mutex);
}

void Server::listen( const std::string &port )
{
  int fd = open_listenfd( port.c_str() );
  if ( fd < 0 ) {
    throw CommException("Failed to open listening socket on port " + port);
  }
  m_listenfd = fd;
}

void Server::server_loop()
{
  // Continuously accept connections and spawn a thread for each
  while (true) {
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    int client_fd = accept(m_listenfd, (SA *)&clientaddr, &clientlen);
    if (client_fd < 0) {
      log_error("Accept failed");
      continue; // possibly continue or break depending on whether we want to shut down
    }

    ClientConnection *client = new ClientConnection(this, client_fd);
    pthread_t thr_id;
    if ( pthread_create( &thr_id, nullptr, client_worker, client ) != 0 ) {
      log_error("Could not create client thread");
      // If thread creation fails, close client_fd and delete client
      Close(client_fd);
      delete client;
      continue;
    }
  }
}

void *Server::client_worker( void *arg )
{
  // Detach this thread so its resources are reclaimed automatically
  pthread_detach(pthread_self());

  std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  client->chat_with_client();
  return nullptr;
}

void Server::log_error( const std::string &what )
{
  std::cerr << "Error: " << what << "\n";
}

void Server::create_table( const std::string &name )
{
  // Validate table name as an identifier
  if (name.empty() || !( (name[0]>='a' && name[0]<='z') || (name[0]>='A' && name[0]<='Z') )) {
    throw OperationException("Invalid table name: " + name);
  }
  for (size_t i=1; i<name.size(); i++) {
    char c = name[i];
    if (!( (c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='_' )) {
      throw OperationException("Invalid table name: " + name);
    }
  }

  Guard g(m_tables_mutex);
  if (m_tables.find(name) != m_tables.end()) {
    throw OperationException("Table already exists: " + name);
  }

  Table *t = new Table(name);
  m_tables[name] = t;
}

Table *Server::find_table( const std::string &name )
{
  // Caller must hold m_tables_mutex
  auto it = m_tables.find(name);
  if (it == m_tables.end())
    return nullptr;
  return it->second;
}

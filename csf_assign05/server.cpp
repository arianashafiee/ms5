#include <iostream>
#include <cassert>
#include <stdexcept>
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
  // Clean up tables
  lock_tables_map();
  for (auto &pair : m_tables) {
    delete pair.second;
  }
  m_tables.clear();
  unlock_tables_map();

  pthread_mutex_destroy(&m_tables_mutex);
}

void Server::listen(const std::string &port)
{
  int fd = open_listenfd(port.c_str());
  if (fd < 0) {
    log_error("Could not open listen socket on port " + port);
    throw CommException("Failed to open listen socket");
  }
  m_listenfd = fd;
}

void Server::server_loop()
{
  if (m_listenfd < 0) {
    log_error("Listen socket not initialized");
    throw CommException("Server listen failed (no socket)");
  }

  while (true) {
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int client_fd = accept(m_listenfd, (SA *)&clientaddr, &clientlen);
    if (client_fd < 0) {
      // Just log and continue
      log_error("Accept failed");
      continue;
    }

    ClientConnection *client = new ClientConnection(this, client_fd);
    pthread_t thr_id;
    if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
      log_error("Could not create client thread");
      Close(client_fd);
      delete client;
      continue;
    }
    // Detach the thread so that it will clean up after itself
    pthread_detach(thr_id);
  }
}

void *Server::client_worker(void *arg)
{
  std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));
  try {
    client->chat_with_client();
  } catch (std::exception &ex) {
    // If unexpected exception, just end the connection
  }
  return nullptr;
}

void Server::log_error(const std::string &what)
{
  std::cerr << "Error: " << what << "\n";
}

void Server::create_table(const std::string &name)
{
  // caller must hold lock
  if (m_tables.find(name) != m_tables.end()) {
    throw OperationException("Table already exists");
  }
  Table *t = new Table(name);
  m_tables[name] = t;
}

Table *Server::find_table(const std::string &name)
{
  auto it = m_tables.find(name);
  if (it == m_tables.end()) {
    return nullptr;
  }
  return it->second;
}

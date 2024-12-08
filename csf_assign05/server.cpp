#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <pthread.h>
#include "csapp.h"
#include "exceptions.h"
#include "guard.h"
#include "server.h"

Server::Server()
  : m_tables(), m_mutex()
{
  pthread_mutex_init(&m_mutex, nullptr);
}

Server::~Server() {
  pthread_mutex_destroy(&m_mutex);
}

void Server::listen(const std::string &port) {
  int listenfd = Open_listenfd(port.c_str());
  if (listenfd < 0) throw CommException("Failed to open listening socket");
  m_listen_fd = listenfd;
}

void Server::server_loop() {
  while (true) {
    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connfd = Accept(m_listen_fd, (SA *)&client_addr, &client_len);
    if (connfd < 0) {
      log_error("Failed to accept connection");
      continue;
    }

    ClientConnection *client = new ClientConnection(this, connfd);
    pthread_t thr_id;
    if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
      log_error("Failed to create thread");
      delete client;
    }
  }
}

void *Server::client_worker(void *arg) {
  std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));
  try {
    client->chat_with_client();
  } catch (const std::exception &e) {
    client->log_error(e.what());
  }
  return nullptr;
}

void Server::create_table(const std::string &name) {
  Guard guard(m_mutex);
  if (m_tables.count(name)) throw OperationException("Table already exists");
  m_tables[name] = std::make_unique<Table>(name);
}

Table *Server::find_table(const std::string &name) {
  Guard guard(m_mutex);
  auto it = m_tables.find(name);
  if (it == m_tables.end()) throw OperationException("Table not found");
  return it->second.get();
}

void Server::log_error(const std::string &what) {
  std::cerr << "Error: " << what << std::endl;
}

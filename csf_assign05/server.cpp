#include "server.h"
#include "client_connection.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

// Constructor
Server::Server() : m_tables() {
    pthread_mutex_init(&m_mutex, nullptr);
}

// Destructor
Server::~Server() {
    pthread_mutex_destroy(&m_mutex);
}

// Listen for incoming connections
void Server::listen(const std::string &port) {
    m_listen_fd = open_listenfd(port.c_str());
    if (m_listen_fd < 0) {
        throw std::runtime_error("Failed to open listening socket");
    }
}

// Server loop to accept client connections
void Server::server_loop() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        int connfd = Accept(m_listen_fd, (SA *)&client_addr, &client_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        auto *client = new ClientConnection(connfd, this);
        pthread_t tid;
        pthread_create(&tid, nullptr, client_worker, client);
        pthread_detach(tid);
    }
}

// Thread worker for handling clients
void *Server::client_worker(void *arg) {
    auto *client = static_cast<ClientConnection *>(arg);
    try {
        client->chat_with_client();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        delete client;
    }
    return nullptr;
}

// Create a new table
void Server::create_table(const std::string &name) {
    pthread_mutex_lock(&m_mutex);
    if (m_tables.find(name) != m_tables.end()) {
        pthread_mutex_unlock(&m_mutex);
        throw std::runtime_error("Table already exists");
    }
    m_tables.emplace(name, Table(name));
    pthread_mutex_unlock(&m_mutex);
}

// Find a table by name
Table *Server::find_table(const std::string &name) {
    pthread_mutex_lock(&m_mutex);
    auto it = m_tables.find(name);
    if (it == m_tables.end()) {
        pthread_mutex_unlock(&m_mutex);
        return nullptr;
    }
    pthread_mutex_unlock(&m_mutex);
    return &it->second;
}

#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <memory>
#include <cstring>
#include "csapp.h"
#include "server.h"
#include "client_connection.h"

#include "server.h"
#include <iostream>
#include <stdexcept>
#include "csapp.h"

Server::Server()
    : m_listen_fd(-1) {
    pthread_mutex_init(&m_table_lock, nullptr); // Initialize the mutex
}

Server::~Server() {
    if (m_listen_fd >= 0) {
        close(m_listen_fd); // Close the listening socket
    }
    pthread_mutex_destroy(&m_table_lock); // Destroy the mutex
}

void Server::listen(const std::string &port) {
    m_listen_fd = open_listenfd(port.c_str());
    if (m_listen_fd < 0) {
        throw std::runtime_error("Failed to open listening socket on port " + port);
    }
}

void Server::server_loop() {
    while (true) {
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(m_listen_fd, (SA *)&client_addr, &client_len);
        if (client_fd < 0) {
            log_error("Failed to accept client connection");
            continue;
        }

        auto *client = new ClientConnection(this, client_fd);
        pthread_t thr_id;
        if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
            log_error("Could not create client thread");
            delete client;
            close(client_fd);
        } else {
            pthread_detach(thr_id); // Detach the thread
        }
    }
}

void *Server::client_worker(void *arg) {
    std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));
    client->chat_with_client();
    return nullptr;
}

void Server::log_error(const std::string &what) {
    std::cerr << "Error: " << what << std::endl;
}

void Server::create_table(const std::string &name) {
    pthread_mutex_lock(&m_table_lock);
    if (m_tables.find(name) != m_tables.end()) {
        pthread_mutex_unlock(&m_table_lock);
        throw std::runtime_error("Table already exists: " + name);
    }
    m_tables[name] = std::make_unique<Table>(name);
    pthread_mutex_unlock(&m_table_lock);
}

Table *Server::find_table(const std::string &name) {
    pthread_mutex_lock(&m_table_lock);
    auto it = m_tables.find(name);
    Table *table = (it != m_tables.end()) ? it->second.get() : nullptr;
    pthread_mutex_unlock(&m_table_lock);
    return table;
}

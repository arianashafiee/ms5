#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <memory>
#include <cstring>
#include "csapp.h"
#include "server.h"
#include "client_connection.h"

Server::Server()
{
    // Initialize member variables if needed (e.g., data structures for tables)
}

Server::~Server()
{
    // Clean up resources if any
}

void Server::listen(const std::string &port)
{
    int listenfd = open_listenfd(port.c_str());
    if (listenfd < 0)
    {
        log_error("Failed to open listening socket on port " + port);
        return;
    }

    m_listen_fd = listenfd;
}

void Server::server_loop()
{
    while (true)
    {
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(m_listen_fd, (SA *)&client_addr, &client_len);

        if (client_fd < 0)
        {
            log_error("Failed to accept client connection");
            continue;
        }

        // Spawn a new thread for the client
        pthread_t thr_id;
        ClientConnection *client = new ClientConnection(this, client_fd);

        if (pthread_create(&thr_id, nullptr, client_worker, client) != 0)
        {
            log_error("Could not create client thread");
            delete client;
            close(client_fd);
        }
        else
        {
            pthread_detach(thr_id); // Detach thread to avoid memory leaks
        }
    }
}

void *Server::client_worker(void *arg)
{
    std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));
    client->chat_with_client();
    return nullptr;
}

void Server::log_error(const std::string &what)
{
    std::cerr << "Error: " << what << std::endl;
}

#include <iostream>
#include <sstream>
#include <string>
#include "client_connection.h"
#include "message.h"
#include "message_serialization.h"
#include "exceptions.h"
#include "server.h" // Include the full definition of Server

ClientConnection::ClientConnection(Server *server, int client_fd)
    : m_server(server), m_client_fd(client_fd) {
    rio_readinitb(&m_fdbuf, m_client_fd);
}

ClientConnection::~ClientConnection() {
    close(m_client_fd); // Ensure socket is closed
}

void ClientConnection::chat_with_client() {
    try {
        while (true) {
            char buf[Message::MAX_ENCODED_LEN];
            ssize_t n = rio_readlineb(&m_fdbuf, buf, Message::MAX_ENCODED_LEN);

            if (n <= 0)
                throw CommException("Client disconnected or read error");

            std::string input(buf);
            Message request;
            MessageSerialization::decode(input, request);

            if (!request.is_valid())
                throw InvalidMessage("Invalid message received");

            process_request(request);
        }
    } catch (const InvalidMessage &e) {
        send_response(MessageType::ERROR, e.what());
    } catch (const CommException &e) {
        if (m_server) {
            m_server->log_error(e.what()); // Log error using the full Server definition
        }
    } catch (const std::exception &e) {
        send_response(MessageType::ERROR, e.what());
    }
}

void ClientConnection::process_request(const Message &request) {
    if (request.get_message_type() == MessageType::LOGIN) {
        // Example implementation for LOGIN
        send_response(MessageType::OK, "");
    } else if (request.get_message_type() == MessageType::BYE) {
        send_response(MessageType::OK, "");
        throw CommException("Client terminated connection");
    } else {
        // Handle other commands like CREATE, PUSH, POP, etc.
        send_response(MessageType::OK, "");
    }
}

void ClientConnection::send_response(MessageType type, const std::string &content) {
    Message response(type, {content});
    std::string encoded;
    MessageSerialization::encode(response, encoded); // Use encode with two arguments
    rio_writen(m_client_fd, encoded.c_str(), encoded.size());
}

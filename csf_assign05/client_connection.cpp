#include <iostream>
#include <sstream>
#include <string>
#include "client_connection.h"
#include "message.h"
#include "message_serialization.h"
#include "exceptions.h"

ClientConnection::ClientConnection(Server *server, int client_fd)
    : m_server(server), m_client_fd(client_fd)
{
    rio_readinitb(&m_fdbuf, m_client_fd);
}

ClientConnection::~ClientConnection()
{
    close(m_client_fd); // Ensure socket is closed
}

void ClientConnection::chat_with_client()
{
    try
    {
        while (true)
        {
            char buf[Message::MAX_ENCODED_LEN];
            ssize_t n = rio_readlineb(&m_fdbuf, buf, Message::MAX_ENCODED_LEN);

            if (n <= 0)
                throw CommException("Client disconnected or read error");

            std::string input(buf);
            Message request = MessageSerialization::decode(input);

            if (!request.is_valid())
                throw InvalidMessage("Invalid message received");

            process_request(request);
        }
    }
    catch (const InvalidMessage &e)
    {
        send_response(Message::Type::ERROR, e.what());
    }
    catch (const CommException &e)
    {
        m_server->log_error(e.what());
    }
    catch (const std::exception &e)
    {
        send_response(Message::Type::ERROR, e.what());
    }
}

void ClientConnection::process_request(const Message &request)
{
    if (request.get_command() == "LOGIN")
    {
        // Example implementation for LOGIN
        send_response(Message::Type::OK, "");
    }
    else if (request.get_command() == "BYE")
    {
        send_response(Message::Type::OK, "");
        throw CommException("Client terminated connection");
    }
    else
    {
        // Handle other commands like CREATE, PUSH, POP, etc.
        send_response(Message::Type::OK, "");
    }
}

void ClientConnection::send_response(Message::Type type, const std::string &content)
{
    Message response(type, content);
    std::string encoded = MessageSerialization::encode(response);
    rio_writen(m_client_fd, encoded.c_str(), encoded.size());
}

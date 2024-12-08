#include <iostream>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"
#include "server.h"
#include "exceptions.h"
#include "client_connection.h"

ClientConnection::ClientConnection(Server *server, int client_fd)
    : m_server(server), m_client_fd(client_fd) {
  rio_readinitb(&m_fdbuf, client_fd);
}

ClientConnection::~ClientConnection() {
  Close(m_client_fd);
}

void ClientConnection::chat_with_client() {
  while (true) {
    char buffer[Message::MAX_ENCODED_LEN];
    if (Rio_readlineb(&m_fdbuf, buffer, Message::MAX_ENCODED_LEN) <= 0) break;

    Message request;
    MessageSerialization::decode(buffer, request);
    Message response;

    try {
      handle_request(request, response);
    } catch (const std::exception &e) {
      response.set_message_type(MessageType::ERROR);
      response.push_arg(e.what());
    }

    std::string encoded_response;
    MessageSerialization::encode(response, encoded_response);
    Rio_writen(m_client_fd, encoded_response.c_str(), encoded_response.size());
  }
}

void ClientConnection::handle_request(const Message &request, Message &response) {
  switch (request.get_message_type()) {
    case MessageType::CREATE:
      m_server->create_table(request.get_table());
      response.set_message_type(MessageType::OK);
      break;

    case MessageType::SET: {
      Table *table = m_server->find_table(request.get_table());
      table->lock();
      table->set(request.get_key(), request.get_value());
      table->unlock();
      response.set_message_type(MessageType::OK);
      break;
    }

    case MessageType::GET: {
      Table *table = m_server->find_table(request.get_table());
      table->lock();
      std::string value = table->get(request.get_key());
      table->unlock();
      response.set_message_type(MessageType::DATA);
      response.push_arg(value);
      break;
    }

    case MessageType::BYE:
      response.set_message_type(MessageType::OK);
      throw std::runtime_error("Client disconnected");

    default:
      throw InvalidMessage("Unknown command");
  }
}
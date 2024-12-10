#include "client_connection.h"
#include "server.h"
#include "exceptions.h"
#include "message_serialization.h"
#include <stdexcept>
#include <memory>
#include <iostream>

ClientConnection::ClientConnection(Server *server, int client_fd)
  : m_server(server), m_client_fd(client_fd), m_inTransaction(false)
{
  rio_readinitb(&m_fdbuf, m_client_fd);
}

ClientConnection::~ClientConnection()
{
  Close(m_client_fd);
}

void ClientConnection::chat_with_client()
{
  bool done = false;
  bool logged_in = false;

  try {
    while (!done) {
      char buffer[Message::MAX_ENCODED_LEN];
      ssize_t rc = rio_readlineb(&m_fdbuf, buffer, Message::MAX_ENCODED_LEN);
      if (rc == 0) {
        // Client closed connection
        done = true;
        break;
      }
      std::string line(buffer);

      Message request;
      try {
        MessageSerialization::decode(line, request);

        // First request must be LOGIN
        if (!logged_in && request.get_message_type() != MessageType::LOGIN) {
          throw InvalidMessage("First message must be LOGIN");
        }

        switch (request.get_message_type()) {
          case MessageType::LOGIN:
            handle_LOGIN(request, logged_in);
            break;
          case MessageType::CREATE:
            handle_CREATE(request);
            break;
          case MessageType::PUSH:
            handle_PUSH(request);
            break;
          case MessageType::POP:
            handle_POP(request);
            break;
          case MessageType::TOP:
            handle_TOP(request);
            break;
          case MessageType::SET:
            handle_SET(request);
            break;
          case MessageType::GET:
            handle_GET(request);
            break;
          case MessageType::ADD:
            handle_ADD(request);
            break;
          case MessageType::SUB:
            handle_SUB(request);
            break;
          case MessageType::MUL:
            handle_MUL(request);
            break;
          case MessageType::DIV:
            handle_DIV(request);
            break;
          case MessageType::BEGIN:
            handle_BEGIN(request);
            break;
          case MessageType::COMMIT:
            handle_COMMIT(request);
            break;
          case MessageType::BYE:
            handle_BYE(request, done);  // Process BYE and ensure OK is sent
            break;
          default:
            throw InvalidMessage("Unknown request message");
        }

      } catch (InvalidMessage &imex) {
        send_error(imex.what());
        done = true;
      } catch (OperationException &opex) {
        if (m_inTransaction) {
          rollback_transaction();
        }
        send_failed(opex.what());
      } catch (FailedTransaction &ftex) {
        rollback_transaction();
        send_failed(ftex.what());
      }
    }
  } catch (CommException &cex) {
    // Communication error: just end silently
  }

  if (m_inTransaction) {
    rollback_transaction();
  }
}

void ClientConnection::handle_BYE(const Message &msg, bool &done)
{
  (void)msg;
  send_ok();  // Ensure OK response is sent
  done = true;  // Mark as done to terminate connection after response
}

void ClientConnection::send_ok()
{
  send_response(MessageType::OK);
}

void ClientConnection::send_failed(const std::string &reason)
{
  send_response(MessageType::FAILED, reason);
}

void ClientConnection::send_error(const std::string &reason)
{
  send_response(MessageType::ERROR, reason);
}

void ClientConnection::send_response(MessageType type, const std::string &arg)
{
  Message msg(type);
  if (!arg.empty()) {
    msg.push_arg(arg);
  }
  std::string encoded;
  MessageSerialization::encode(msg, encoded);

  // Ensure the response is fully written
  if (Rio_writen(m_client_fd, encoded.c_str(), encoded.size()) != (ssize_t)encoded.size()) {
    throw CommException("Failed to send response");
  }
}

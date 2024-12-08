#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <set>
#include <string>
#include "message.h"
#include "csapp.h"

class Server; // forward declaration

class ClientConnection {
private:
  Server *m_server;
  int m_client_fd;
  rio_t m_fdbuf;

  // copy constructor and assignment operator are prohibited
  ClientConnection(const ClientConnection &);
  ClientConnection &operator=(const ClientConnection &);

  // Helper methods
  void process_request(const Message &request);
  void send_response(Message::MessageType type, const std::string &content);

public:
  ClientConnection(Server *server, int client_fd);
  ~ClientConnection();

  void chat_with_client();
};

#endif // CLIENT_CONNECTION_H

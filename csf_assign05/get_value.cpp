#include <iostream>
#include <string>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"

int main(int argc, char **argv)
{
  if (argc != 6) {
    std::cerr << "Usage: ./get_value <hostname> <port> <username> <table> <key>\n";
    return 1;
  }

  std::string hostname = argv[1];
  std::string port = argv[2];
  std::string username = argv[3];
  std::string table = argv[4];
  std::string key = argv[5];

  int clientfd;
  rio_t rio;

  try {
    // Establish a connection to the server
    clientfd = Open_clientfd(hostname.c_str(), port.c_str());
    Rio_readinitb(&rio, clientfd);

    // Send LOGIN message
    Message login_msg(MessageType::LOGIN, {username});
    std::string encoded_login;
    MessageSerialization::encode(login_msg, encoded_login);
    Rio_writen(clientfd, encoded_login.c_str(), encoded_login.size());

    // Read response to LOGIN
    char buffer[Message::MAX_ENCODED_LEN];
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    Message response;
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() != MessageType::OK) {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Send GET message
    Message get_msg(MessageType::GET, {table, key});
    std::string encoded_get;
    MessageSerialization::encode(get_msg, encoded_get);
    Rio_writen(clientfd, encoded_get.c_str(), encoded_get.size());

    // Read response to GET
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() != MessageType::OK) {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Send TOP message
    Message top_msg(MessageType::TOP);
    std::string encoded_top;
    MessageSerialization::encode(top_msg, encoded_top);
    Rio_writen(clientfd, encoded_top.c_str(), encoded_top.size());

    // Read response to TOP
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() == MessageType::DATA) {
      std::cout << response.get_value() << "\n";
    } else {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Send BYE message
    Message bye_msg(MessageType::BYE);
    std::string encoded_bye;
    MessageSerialization::encode(bye_msg, encoded_bye);
    Rio_writen(clientfd, encoded_bye.c_str(), encoded_bye.size());

    // Close the connection
    Close(clientfd);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}

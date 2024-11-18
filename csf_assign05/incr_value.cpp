#include <iostream>
#include <string>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"

int main(int argc, char **argv) {
  if (argc != 6 && (argc != 7 || std::string(argv[1]) != "-t")) {
    std::cerr << "Usage: ./incr_value [-t] <hostname> <port> <username> <table> <key>\n";
    std::cerr << "Options:\n";
    std::cerr << "  -t      execute the increment as a transaction\n";
    return 1;
  }

  int count = 1;
  bool use_transaction = false;

  if (argc == 7) {
    use_transaction = true;
    count = 2;
  }

  std::string hostname = argv[count++];
  std::string port = argv[count++];
  std::string username = argv[count++];
  std::string table = argv[count++];
  std::string key = argv[count++];

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

    // Begin transaction if -t is specified
    if (use_transaction) {
      Message begin_msg(MessageType::BEGIN);
      std::string encoded_begin;
      MessageSerialization::encode(begin_msg, encoded_begin);
      Rio_writen(clientfd, encoded_begin.c_str(), encoded_begin.size());

      // Read response to BEGIN
      Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
      MessageSerialization::decode(buffer, response);
      if (response.get_message_type() != MessageType::OK) {
        std::cerr << "Error: " << response.get_quoted_text() << "\n";
        Close(clientfd);
        return 1;
      }
    }

    // Send GET message to retrieve the current value
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

    // Send TOP message to get the value from the operand stack
    Message top_msg(MessageType::TOP);
    std::string encoded_top;
    MessageSerialization::encode(top_msg, encoded_top);
    Rio_writen(clientfd, encoded_top.c_str(), encoded_top.size());

    // Read response to TOP
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() != MessageType::DATA) {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Increment the retrieved value
    int current_value = std::stoi(response.get_value());
    int incremented_value = current_value + 1;

    // Push the incremented value onto the operand stack
    Message push_msg(MessageType::PUSH, {std::to_string(incremented_value)});
    std::string encoded_push;
    MessageSerialization::encode(push_msg, encoded_push);
    Rio_writen(clientfd, encoded_push.c_str(), encoded_push.size());

    // Read response to PUSH
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() != MessageType::OK) {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Send SET message to update the value in the table
    Message set_msg(MessageType::SET, {table, key});
    std::string encoded_set;
    MessageSerialization::encode(set_msg, encoded_set);
    Rio_writen(clientfd, encoded_set.c_str(), encoded_set.size());

    // Read response to SET
    Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
    MessageSerialization::decode(buffer, response);
    if (response.get_message_type() != MessageType::OK) {
      std::cerr << "Error: " << response.get_quoted_text() << "\n";
      Close(clientfd);
      return 1;
    }

    // Commit transaction if -t is specified
    if (use_transaction) {
      Message commit_msg(MessageType::COMMIT);
      std::string encoded_commit;
      MessageSerialization::encode(commit_msg, encoded_commit);
      Rio_writen(clientfd, encoded_commit.c_str(), encoded_commit.size());

      // Read response to COMMIT
      Rio_readlineb(&rio, buffer, Message::MAX_ENCODED_LEN);
      MessageSerialization::decode(buffer, response);
      if (response.get_message_type() != MessageType::OK) {
        std::cerr << "Error: " << response.get_quoted_text() << "\n";
        Close(clientfd);
        return 1;
      }
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

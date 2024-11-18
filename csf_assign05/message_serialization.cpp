#include <utility>
#include <sstream>
#include <cassert>
#include <ostream>
#include <map>
#include "exceptions.h"
#include "message_serialization.h"

void MessageSerialization::encode(const Message &msg, std::string &encoded_msg) {
  std::ostringstream oss;

  // Convert MessageType to int explicitly before appending
  oss << static_cast<int>(msg.get_message_type());

  // Append arguments
  for (unsigned i = 0; i < msg.get_num_args(); ++i) {
    const std::string &arg = msg.get_arg(i);

    // Quote arguments containing spaces or quotes
    if (arg.find(' ') != std::string::npos || arg.find('"') != std::string::npos) {
      oss << " \"" << arg << "\"";
    } else {
      oss << " " << arg;
    }
  }

  // Add a newline to terminate the message
  oss << "\n";
  encoded_msg = oss.str();

  // Validate length constraint
  if (encoded_msg.size() > Message::MAX_ENCODED_LEN) {
    throw InvalidMessage("Encoded message exceeds maximum length");
  }
}


void MessageSerialization::decode(const std::string &encoded_msg_, Message &msg) {
  if (encoded_msg_.empty() || encoded_msg_.back() != '\n') {
    throw InvalidMessage("Encoded message must end with a newline");
  }

  std::string encoded_msg = encoded_msg_;
  encoded_msg.pop_back(); // Remove the newline

  std::istringstream stream(encoded_msg);
  std::string token;

  // Parse the message type
  if (!(stream >> token)) {
    throw InvalidMessage("Encoded message is empty or malformed");
  }

  int message_type;
  try {
    message_type = std::stoi(token);
  } catch (const std::invalid_argument &) {
    throw InvalidMessage("Invalid message type");
  }

  if (message_type < static_cast<int>(MessageType::NONE) ||
      message_type > static_cast<int>(MessageType::DATA)) {
    throw InvalidMessage("Unknown message type");
  }

  msg.set_message_type(static_cast<MessageType>(message_type));

  // Parse arguments
  std::vector<std::string> args;
  while (stream >> token) {
    if (token.front() == '"') {
      std::string quoted_arg = token.substr(1); // Remove leading quote
      while (!quoted_arg.empty() && quoted_arg.back() != '"') {
        if (!(stream >> token)) {
          throw InvalidMessage("Unterminated quoted argument");
        }
        quoted_arg += " " + token;
      }
      if (quoted_arg.back() != '"') {
        throw InvalidMessage("Unterminated quoted argument");
      }
      quoted_arg.pop_back(); // Remove trailing quote
      args.push_back(quoted_arg);
    } else {
      args.push_back(token); // Add directly if not quoted
    }
  }

  for (const auto &arg : args) {
    msg.push_arg(arg);
  }

  if (!msg.is_valid()) {
    throw InvalidMessage("Decoded message is invalid");
  }
}

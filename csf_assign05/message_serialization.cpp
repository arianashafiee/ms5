#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include "exceptions.h"
#include "message_serialization.h"

void MessageSerialization::encode(const Message &msg, std::string &encoded_msg) {
  // Start with the message type
  encoded_msg = std::to_string(static_cast<int>(msg.get_message_type()));

  // Append arguments to the encoded message
  for (unsigned i = 0; i < msg.get_num_args(); ++i) {
    const std::string &arg = msg.get_arg(i);

    // Check if the argument needs to be quoted
    if (arg.find(' ') != std::string::npos || arg.find('"') != std::string::npos) {
      encoded_msg += " \"" + arg + "\""; // Add quotes around the argument
    } else {
      encoded_msg += " " + arg; // Add argument directly
    }
  }

  // Add newline to indicate the end of the message
  encoded_msg += "\n";

  // Validate that the encoded message does not exceed the maximum length
  if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
    throw InvalidMessage("Encoded message exceeds maximum length");
  }
}


void MessageSerialization::decode(const std::string &encoded_msg_, Message &msg) {
  // Ensure the message ends with a newline
  if (encoded_msg_.empty() || encoded_msg_.back() != '\n') {
    throw InvalidMessage("Encoded message must end with a newline");
  }

  // Create a working copy without the trailing newline
  std::string encoded_msg = encoded_msg_;
  encoded_msg.pop_back();

  // Use a string stream to tokenize the message
  std::istringstream stream(encoded_msg);
  std::string token;

  // Read and parse the message type
  if (!(stream >> token)) {
    throw InvalidMessage("Encoded message is empty or malformed");
  }

  int message_type;
  try {
    message_type = std::stoi(token);
  } catch (const std::invalid_argument &) {
    throw InvalidMessage("Invalid message type");
  }

  // Validate the message type
  if (message_type < static_cast<int>(MessageType::NONE) ||
      message_type > static_cast<int>(MessageType::DATA)) {
    throw InvalidMessage("Unknown message type");
  }

  msg.set_message_type(static_cast<MessageType>(message_type));

  // Parse the arguments
  std::vector<std::string> args;
  while (stream >> token) {
    // Handle quoted arguments
    if (token.front() == '"') {
      std::string quoted_arg = token.substr(1); // Remove leading quote
      while (!quoted_arg.empty() && quoted_arg.back() != '"') {
        if (!(stream >> token)) {
          throw InvalidMessage("Unterminated quoted argument");
        }
        quoted_arg += " " + token;
      }

      if (quoted_arg.empty() || quoted_arg.back() != '"') {
        throw InvalidMessage("Unterminated quoted argument");
      }

      quoted_arg.pop_back(); // Remove trailing quote
      args.push_back(quoted_arg);
    } else {
      args.push_back(token); // Add non-quoted arguments
    }
  }

  // Add arguments to the message
  for (const auto &arg : args) {
    msg.push_arg(arg);
  }

  // Validate the decoded message
  if (!msg.is_valid()) {
    throw InvalidMessage("Decoded message is invalid");
  }
}

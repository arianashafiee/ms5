#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include <regex>
#include "exceptions.h"
#include "message_serialization.h"

void MessageSerialization::encode(const Message &msg, std::string &encoded_msg) {
    // Start with the command based on the message type
    static const std::map<MessageType, std::string> type_to_str = {
        {MessageType::LOGIN, "LOGIN"},
        {MessageType::CREATE, "CREATE"},
        {MessageType::PUSH, "PUSH"},
        {MessageType::POP, "POP"},
        {MessageType::TOP, "TOP"},
        {MessageType::SET, "SET"},
        {MessageType::GET, "GET"},
        {MessageType::ADD, "ADD"},
        {MessageType::SUB, "SUB"},
        {MessageType::MUL, "MUL"},
        {MessageType::DIV, "DIV"},
        {MessageType::BEGIN, "BEGIN"},
        {MessageType::COMMIT, "COMMIT"},
        {MessageType::BYE, "BYE"},
        {MessageType::OK, "OK"},
        {MessageType::FAILED, "FAILED"},
        {MessageType::ERROR, "ERROR"},
        {MessageType::DATA, "DATA"}
    };

    auto it = type_to_str.find(msg.get_message_type());
    if (it == type_to_str.end()) {
        throw InvalidMessage("Unknown message type");
    }

    std::ostringstream oss;
    oss << it->second;

    // Append arguments separated by spaces
    for (unsigned i = 0; i < msg.get_num_args(); ++i) {
        oss << " ";
        // Quote any argument containing spaces or special characters
        const std::string &arg = msg.get_arg(i);
        if (arg.find_first_of(" \t\n\"") != std::string::npos) {
            oss << "\"" << arg << "\"";
        } else {
            oss << arg;
        }
    }

    oss << "\n";
    encoded_msg = oss.str();

    // Validate message length
    if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
        throw InvalidMessage("Encoded message exceeds maximum length");
    }
}

void MessageSerialization::decode(const std::string &encoded_msg_, Message &msg) {
    // Check message length
    if (encoded_msg_.length() > Message::MAX_ENCODED_LEN) {
        throw InvalidMessage("Encoded message exceeds maximum length");
    }

    // Ensure the message ends with a newline
    if (encoded_msg_.empty() || encoded_msg_.back() != '\n') {
        throw InvalidMessage("Encoded message lacks terminating newline");
    }

    // Remove the trailing newline for easier processing
    std::string encoded_msg = encoded_msg_;
    encoded_msg.pop_back();

    std::istringstream iss(encoded_msg);
    std::string token;

    // Extract the command (first token)
    if (!(iss >> token)) {
        throw InvalidMessage("Encoded message is empty or invalid");
    }

    static const std::map<std::string, MessageType> str_to_type = {
        {"LOGIN", MessageType::LOGIN},
        {"CREATE", MessageType::CREATE},
        {"PUSH", MessageType::PUSH},
        {"POP", MessageType::POP},
        {"TOP", MessageType::TOP},
        {"SET", MessageType::SET},
        {"GET", MessageType::GET},
        {"ADD", MessageType::ADD},
        {"SUB", MessageType::SUB},
        {"MUL", MessageType::MUL},
        {"DIV", MessageType::DIV},
        {"BEGIN", MessageType::BEGIN},
        {"COMMIT", MessageType::COMMIT},
        {"BYE", MessageType::BYE},
        {"OK", MessageType::OK},
        {"FAILED", MessageType::FAILED},
        {"ERROR", MessageType::ERROR},
        {"DATA", MessageType::DATA}
    };

    auto it = str_to_type.find(token);
    if (it == str_to_type.end()) {
        throw InvalidMessage("Unknown command: " + token);
    }

    msg.set_message_type(it->second);
    msg = Message(it->second);

    // Extract arguments
    std::string arg;
    while (iss >> arg) {
        if (arg.front() == '"') { // Handle quoted arguments
            std::string quoted_arg = arg.substr(1);
            while (iss && quoted_arg.back() != '"') {
                std::string part;
                iss >> part;
                quoted_arg += " " + part;
            }
            if (quoted_arg.back() != '"') {
                throw InvalidMessage("Malformed quoted argument");
            }
            quoted_arg.pop_back(); // Remove trailing quote
            msg.push_arg(quoted_arg);
        } else {
            msg.push_arg(arg);
        }
    }

    // Final validation of message arguments
    if (!msg.is_valid()) {
        throw InvalidMessage("Decoded message is invalid");
    }
}

#ifndef VALUE_STACK_H
#define VALUE_STACK_H

#include <vector>
#include <string>

class ValueStack {
private:
  std::vector<std::string> m_stack; // Internal stack represented as a vector

public:
  ValueStack();
  ~ValueStack();

  bool is_empty() const;              // Check if the stack is empty
  void push(const std::string &value); // Push a value onto the stack

  // Note: get_top() and pop() should throw OperationException
  // if called when the stack is empty

  std::string get_top() const; // Get the value at the top of the stack
  void pop();                  // Remove the value at the top of the stack
};

#endif // VALUE_STACK_H

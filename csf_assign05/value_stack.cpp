#include "value_stack.h"
#include "exceptions.h"

ValueStack::ValueStack()
  : m_stack() // Initialize the stack as an empty vector
{
}

ValueStack::~ValueStack()
{
}

bool ValueStack::is_empty() const
{
  // Check if the stack is empty
  return m_stack.empty();
}

void ValueStack::push(const std::string &value)
{
  // Push a value onto the stack
  m_stack.push_back(value);
}

std::string ValueStack::get_top() const
{
  if (is_empty()) {
    // Throw exception if stack is empty
    throw OperationException("Stack is empty, cannot get top value");
  }
  // Return the top value
  return m_stack.back();
}

void ValueStack::pop()
{
  if (is_empty()) {
    // Throw exception if stack is empty
    throw OperationException("Stack is empty, cannot pop value");
  }
  // Remove the top value
  m_stack.pop_back();
}

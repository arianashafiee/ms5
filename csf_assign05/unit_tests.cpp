// Unit tests

#include "message.h"
#include "message_serialization.h"
#include "table.h"
#include "value_stack.h"
#include "exceptions.h"
#include "tctest.h"

struct TestObjs
{
  Message m; // default message

  // some valid messages
  Message login_req;
  Message create_req;
  Message push_req;
  Message pop_req;
  Message set_req;
  Message get_req;
  Message add_req;
  Message mul_req;
  Message sub_req;
  Message div_req;
  Message bye_req;
  Message ok_resp;
  Message failed_resp;
  Message error_resp;
  Message data_resp;
  Message long_get_req;
  Message create_req_2;

  // some invalid messages
  // (meaning that the internal data violates the protocol spec)
  Message invalid_login_req;
  Message invalid_create_req;
  Message invalid_data_resp;

  // Message that is too long to encode
  Message invalid_too_long;

  // Some encoded messages to test decoding
  std::string encoded_login_req;
  std::string encoded_create_req;
  std::string encoded_data_resp;
  std::string encoded_get_req;
  std::string encoded_failed_resp;
  std::string encoded_error_resp;
  std::string encoded_bye_req;

  // Invalid encoded messages to test decoding
  std::string encoded_push_req_no_nl;
  std::string encoded_get_req_too_long;

  // Test object for Table unit tests
  Table *invoices;
  Table *line_items;

  // ValueStack object for testing
  ValueStack valstack;

  TestObjs();
  ~TestObjs();
};

// Create and clean up test fixture
TestObjs *setup();
void cleanup( TestObjs *objs );

// Guard object to ensure that Table is locked and unlocked
// in unit tests. This could be used to manage locking and
// unlocking a Table when autocommit (no multi-table transactions)
// is desired.
class TableGuard {
public:
  Table *m_table;

  TableGuard( Table *table )
    : m_table( table )
  {
    m_table->lock();
  }

  ~TableGuard()
  {
    m_table->unlock();
  }
};

// Prototypes of test functions
void test_message_default_ctor( TestObjs *objs );
void test_message_get_message_type( TestObjs *objs );
void test_message_get_username( TestObjs *objs );
void test_message_get_table( TestObjs *objs );
void test_message_get_value( TestObjs *objs );
void test_message_get_key( TestObjs *objs );
void test_message_is_valid( TestObjs *objs );
void test_message_serialization_encode( TestObjs *objs );
void test_message_serialization_encode_long( TestObjs *objs );
void test_message_serialization_encode_too_long( TestObjs *objs );
void test_message_serialization_decode( TestObjs *objs );
void test_message_serialization_decode_invalid( TestObjs *objs );
void test_table_has_key( TestObjs *objs );
void test_table_get( TestObjs *objs );
void test_table_commit_changes( TestObjs *objs );
void test_table_rollback_changes( TestObjs *objs );
void test_table_commit_and_rollback( TestObjs *objs );
void test_value_stack( TestObjs *objs );
void test_value_stack_exceptions( TestObjs *objs );

int main(int argc, char **argv)
{
  // Allow test name to be specified on the command line
  if ( argc >= 2 )
    tctest_testname_to_execute = argv[1];

  TEST_INIT();

  TEST( test_message_default_ctor );
  TEST( test_message_get_message_type );
  TEST( test_message_get_username );
  TEST( test_message_get_table );
  TEST( test_message_get_value );
  TEST( test_message_get_key );
  TEST( test_message_is_valid );
  TEST( test_message_serialization_encode );
  TEST( test_message_serialization_encode_long );
  TEST( test_message_serialization_encode_too_long );
  TEST( test_message_serialization_decode );
  TEST( test_message_serialization_decode_invalid );
  TEST( test_table_has_key );
  TEST( test_table_get );
  TEST( test_table_commit_changes );
  TEST( test_table_rollback_changes );
  TEST( test_table_commit_and_rollback );
  TEST( test_value_stack );
  TEST( test_value_stack_exceptions );

  TEST_FINI();
}

// Constructor for TestObjs.
// All test fixture objects should be initialized here.
TestObjs::TestObjs()
  : m()

  // Valid messages
  , login_req( MessageType::LOGIN, { "alice" } )
  , create_req( MessageType::CREATE, { "accounts" } )
  , push_req( MessageType::PUSH, { "47374" } )
  , pop_req( MessageType::POP )
  , set_req( MessageType::SET, { "accounts", "acct123" } )
  , get_req( MessageType::GET, { "accounts", "acct123" } )
  , add_req( MessageType::ADD )
  , mul_req( MessageType::MUL )
  , sub_req( MessageType::SUB )
  , div_req( MessageType::DIV )
  , bye_req( MessageType::BYE )
  , ok_resp( MessageType::OK )
  // Note that the quoted_text argument in FAILED and ERROR responses
  // have the quotes stripped off before being incorporated into the
  // enclosing Message object
  , failed_resp( MessageType::FAILED, { "The operation failed" } )
  , error_resp( MessageType::ERROR, { "An error occurred" } )
  , data_resp( MessageType::DATA, { "10012" } )
  , long_get_req( MessageType::GET )
  // underscores in identifiers are legal (if not the first character)
  , create_req_2( MessageType::CREATE, { "line_items" } )

  // Invalid (non-protocol-conforming) messages
  , invalid_login_req( MessageType::LOGIN, { "bob", "extra" } ) // too many args
  , invalid_create_req( MessageType::CREATE, { "8foobar" } ) // arg is not an identifier
  , invalid_data_resp( MessageType::DATA ) // missing argument
  , invalid_too_long( MessageType::SET )

  // Encoded messages (for testing decoding)
  , encoded_login_req( "LOGIN alice\n" )
  , encoded_create_req( "     CREATE   invoices  \n" ) // unusual whitespace, but this is legal
  , encoded_data_resp( "DATA 90125\n" )
  , encoded_get_req( "GET lineitems foobar\n" )
  , encoded_failed_resp( "FAILED \"Something went wrong, shucks!\"\n" )
  , encoded_error_resp( " ERROR \"Wow, something really got messed up\"\n"  )
  , encoded_bye_req( "BYE\n" )

  // Invalid encoded messages
  , encoded_push_req_no_nl( "PUSH 91025" )
  , encoded_get_req_too_long( "GET foo " + std::string( Message::MAX_ENCODED_LEN, 'x' ) )

  // Table objects
  , invoices( new Table( "invoices" ) )
  , line_items( new Table( "line_items" ) )
{
  // This GET request message is just barely small enough to encode
  // (with no room to spare)
  long_get_req.push_arg( std::string( 509, 'y' ) );
  long_get_req.push_arg( std::string( 509, 'y' ) );

  // This SET message is (just barely by 1 character) too large to encode
  invalid_too_long.push_arg( std::string( 509, 'x' ) );
  invalid_too_long.push_arg( std::string( 510, 'x' ) );
}

// Destructor for TestObjs
TestObjs::~TestObjs()
{
  delete invoices;
  delete line_items;
}

TestObjs *setup()
{
  return new TestObjs;
}

void cleanup(TestObjs *objs)
{
  delete objs;
}

void test_message_default_ctor(TestObjs *objs)
{
  ASSERT( MessageType::NONE == objs->m.get_message_type() );
  ASSERT( objs->m.get_num_args() == 0 );
}

void test_message_get_message_type( TestObjs *objs )
{
  ASSERT( MessageType::LOGIN == objs->login_req.get_message_type() );
  ASSERT( MessageType::CREATE == objs->create_req.get_message_type() );
  ASSERT( MessageType::PUSH == objs->push_req.get_message_type() );
  ASSERT( MessageType::POP == objs->pop_req.get_message_type() );
  ASSERT( MessageType::SET == objs->set_req.get_message_type() );
  ASSERT( MessageType::GET == objs->get_req.get_message_type() );
  ASSERT( MessageType::ADD == objs->add_req.get_message_type() );
  ASSERT( MessageType::MUL == objs->mul_req.get_message_type() );
  ASSERT( MessageType::SUB == objs->sub_req.get_message_type() );
  ASSERT( MessageType::DIV == objs->div_req.get_message_type() );
  ASSERT( MessageType::BYE == objs->bye_req.get_message_type() );

  ASSERT( MessageType::OK == objs->ok_resp.get_message_type() );
  ASSERT( MessageType::FAILED == objs->failed_resp.get_message_type() );
  ASSERT( MessageType::ERROR == objs->error_resp.get_message_type() );
  ASSERT( MessageType::DATA == objs->data_resp.get_message_type() );

  ASSERT( MessageType::GET == objs->long_get_req.get_message_type() );
  ASSERT( MessageType::CREATE == objs->create_req_2.get_message_type() );
}

void test_message_get_username( TestObjs *objs )
{
  ASSERT( "alice" == objs->login_req.get_username() );
}

void test_message_get_table( TestObjs *objs )
{
  ASSERT( "accounts" == objs->create_req.get_table() );
  ASSERT( "accounts" == objs->set_req.get_table() );
  ASSERT( "accounts" == objs->get_req.get_table() );
  ASSERT( std::string( 509, 'y' ) == objs->long_get_req.get_table() );
  ASSERT( "line_items" == objs->create_req_2.get_table() );
}

void test_message_get_value( TestObjs *objs )
{
  ASSERT( "47374" == objs->push_req.get_value() );
  ASSERT( "10012" == objs->data_resp.get_value() );
}

void test_message_get_key( TestObjs *objs )
{
  ASSERT( "acct123" == objs->set_req.get_key() );
  ASSERT( "acct123" == objs->get_req.get_key() );
}
void test_message_is_valid(TestObjs *objs)
{
  auto assert_valid = [](const Message &msg, const std::string &msg_name) {
    if (!msg.is_valid()) {
      std::cerr << "FAIL: " << msg_name << " is not valid.\n";
    }
    assert(msg.is_valid());
  };

  auto assert_invalid = [](const Message &msg, const std::string &msg_name) {
    if (msg.is_valid()) {
      std::cerr << "FAIL: " << msg_name << " is valid but should be invalid.\n";
    }
    assert(!msg.is_valid());
  };

  assert_valid(objs->login_req, "login_req");
  assert_valid(objs->create_req, "create_req");
  assert_valid(objs->push_req, "push_req");
  assert_valid(objs->pop_req, "pop_req");
  assert_valid(objs->set_req, "set_req");
  assert_valid(objs->get_req, "get_req");
  assert_valid(objs->add_req, "add_req");
  assert_valid(objs->mul_req, "mul_req");
  assert_valid(objs->sub_req, "sub_req");
  assert_valid(objs->div_req, "div_req");
  assert_valid(objs->bye_req, "bye_req");
  assert_valid(objs->ok_resp, "ok_resp");
  assert_valid(objs->failed_resp, "failed_resp");
  assert_valid(objs->error_resp, "error_resp");
  assert_valid(objs->data_resp, "data_resp");
  assert_valid(objs->long_get_req, "long_get_req");
  assert_valid(objs->create_req_2, "create_req_2");

  assert_invalid(objs->invalid_login_req, "invalid_login_req");
  assert_invalid(objs->invalid_create_req, "invalid_create_req");
  assert_invalid(objs->invalid_data_resp, "invalid_data_resp");
}
void test_message_serialization_encode(TestObjs *objs)
{
  auto assert_encoded = [](const Message &msg, const std::string &expected, const std::string &msg_name) {
    std::string actual;
    MessageSerialization::encode(msg, actual);
    if (actual != expected) {
      std::cerr << "FAIL: Encoding of " << msg_name << " failed.\n";
      std::cerr << "Expected: " << expected << "\n";
      std::cerr << "Actual:   " << actual << "\n";
    }
    assert(actual == expected);
  };

  assert_encoded(objs->login_req, "LOGIN alice\n", "login_req");
  assert_encoded(objs->create_req, "CREATE accounts\n", "create_req");
  assert_encoded(objs->push_req, "PUSH 47374\n", "push_req");
  assert_encoded(objs->pop_req, "POP\n", "pop_req");
  assert_encoded(objs->set_req, "SET accounts acct123\n", "set_req");
  assert_encoded(objs->data_resp, "DATA 10012\n", "data_resp");
}

void test_message_serialization_encode_long(TestObjs *objs)
{
  std::string expected_str = "GET " + std::string(509, 'y') + " " + std::string(509, 'y') + "\n";
  std::string actual_str;
  MessageSerialization::encode(objs->long_get_req, actual_str);
  if (expected_str != actual_str) {
    std::cerr << "FAIL: Encoding of long_get_req failed.\n";
    std::cerr << "Expected: " << expected_str << "\n";
    std::cerr << "Actual:   " << actual_str << "\n";
  }
  assert(expected_str == actual_str);
}
void test_message_serialization_encode_too_long(TestObjs *objs)
{
  try {
    std::string s;
    MessageSerialization::encode(objs->invalid_too_long, s);
    std::cerr << "FAIL: Exception was not thrown for too-long encoded message.\n";
    assert(false); // Fail the test
  } catch (InvalidMessage &ex) {
    std::cerr << "PASS: Exception thrown as expected for too-long encoded message.\n";
  }
}
void test_message_serialization_decode(TestObjs *objs)
{
  auto assert_decoded = [](const std::string &encoded, const Message &expected_msg, const std::string &msg_name) {
    Message actual_msg;
    MessageSerialization::decode(encoded, actual_msg);
    if (actual_msg != expected_msg) { // Assuming operator== is implemented for Message
      std::cerr << "FAIL: Decoding of " << msg_name << " failed.\n";
      std::cerr << "Expected: " << expected_msg.to_string() << "\n"; // Assuming to_string() is implemented
      std::cerr << "Actual:   " << actual_msg.to_string() << "\n";
    }
    assert(actual_msg == expected_msg);
  };

  assert_decoded(objs->encoded_login_req, objs->login_req, "login_req");
  assert_decoded(objs->encoded_create_req, objs->create_req, "create_req");
  assert_decoded(objs->encoded_data_resp, objs->data_resp, "data_resp");
  assert_decoded(objs->encoded_get_req, objs->get_req, "get_req");
  assert_decoded(objs->encoded_failed_resp, objs->failed_resp, "failed_resp");
  assert_decoded(objs->encoded_error_resp, objs->error_resp, "error_resp");
  assert_decoded(objs->encoded_bye_req, objs->bye_req, "bye_req");
}

void test_table_has_key( TestObjs *objs )
{
  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    objs->invoices->set( "abc123", "1000" );
    objs->invoices->set( "xyz456", "1318" );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Changes should be visible even though we haven't committed them
    ASSERT( objs->invoices->has_key( "abc123" ) );
    ASSERT( objs->invoices->has_key( "xyz456" ) );

    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }
}

void test_table_get( TestObjs *objs )
{
  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    objs->invoices->set( "abc123", "1000" );
    objs->invoices->set( "xyz456", "1318" );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Changes should be visible even though we haven't committed them
    ASSERT( "1000" == objs->invoices->get( "abc123" ) );
    ASSERT( "1318" == objs->invoices->get( "xyz456" ) );

    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }
}

void test_table_commit_changes( TestObjs *objs )
{
  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    objs->invoices->set( "abc123", "1000" );
    objs->invoices->set( "xyz456", "1318" );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Changes should be visible even though we haven't committed them
    ASSERT( "1000" == objs->invoices->get( "abc123" ) );
    ASSERT( "1318" == objs->invoices->get( "xyz456" ) );

    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Commit changes
    objs->invoices->commit_changes();

    // Changes should still be visible
    ASSERT( "1000" == objs->invoices->get( "abc123" ) );
    ASSERT( "1318" == objs->invoices->get( "xyz456" ) );

    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }
}

void test_table_rollback_changes( TestObjs *objs )
{
  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    objs->invoices->set( "abc123", "1000" );
    objs->invoices->set( "xyz456", "1318" );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Changes should be visible even though we haven't committed them
    ASSERT( "1000" == objs->invoices->get( "abc123" ) );
    ASSERT( "1318" == objs->invoices->get( "xyz456" ) );

    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Rollback changes
    objs->invoices->rollback_changes();
  }

  {
    TableGuard g( objs->invoices ); // ensure table is locked and unlocked

    // Table should be empty again!
    ASSERT( !objs->invoices->has_key( "abc123" ) );
    ASSERT( !objs->invoices->has_key( "xyz456" ) );
    ASSERT( !objs->invoices->has_key( "nonexistent" ) );
  }
}

// Test that changes can be committed, then more modifications can be
// done and then rolled back, and the originally committed data is still
// there.
void test_table_commit_and_rollback( TestObjs *objs )
{
  // Add some data
  {
    TableGuard g( objs->line_items );

    objs->line_items->set( "apples", "100" );
    objs->line_items->set( "bananas", "150" );
  }

  // Commit changes
  {
    TableGuard g( objs->line_items );

    objs->line_items->commit_changes();
  }

  // Ensure that data is there
  {
    TableGuard g( objs->line_items );

    ASSERT( "100" == objs->line_items->get( "apples" ) );
    ASSERT( "150" == objs->line_items->get( "bananas" ) );
  }

  // Add more data
  {
    TableGuard g( objs->line_items );

    objs->line_items->set( "oranges", "220" );
  }

  // Ensure that data is there
  {
    TableGuard g( objs->line_items );

    ASSERT( "100" == objs->line_items->get( "apples" ) );
    ASSERT( "150" == objs->line_items->get( "bananas" ) );
    ASSERT( "220" == objs->line_items->get( "oranges" ) );
  }

  // Rollback most recent change
  {
    TableGuard g( objs->line_items );

    objs->line_items->rollback_changes();
  }

  // Original data should still be there (since it was committed),
  // but pending change shouldn't be there
  {
    TableGuard g( objs->line_items );

    ASSERT( "100" == objs->line_items->get( "apples" ) );
    ASSERT( "150" == objs->line_items->get( "bananas" ) );
    ASSERT( !objs->line_items->has_key( "oranges" ) );
  }
}

void test_value_stack( TestObjs *objs )
{
  // stack should be empty initially
  ASSERT( objs->valstack.is_empty() );

  // push some values
  objs->valstack.push( "foo" );
  ASSERT( !objs->valstack.is_empty() );
  objs->valstack.push( "bar" );
  ASSERT( !objs->valstack.is_empty() );
  objs->valstack.push( "12345" );
  ASSERT( !objs->valstack.is_empty() );

  // verify that pushed values can be accessed and popped off
  ASSERT( "12345" == objs->valstack.get_top() );
  objs->valstack.pop();
  ASSERT( !objs->valstack.is_empty() );
  ASSERT( "bar" == objs->valstack.get_top() );
  objs->valstack.pop();
  ASSERT( !objs->valstack.is_empty() );
  ASSERT( "foo" == objs->valstack.get_top() );
  objs->valstack.pop();

  // stack should be empty now
  ASSERT( objs->valstack.is_empty() );
}

void test_value_stack_exceptions( TestObjs *objs )
{
  ASSERT( objs->valstack.is_empty() );

  try {
    objs->valstack.get_top();
    FAIL( "ValueStack didn't throw exception for get_top() on empty stack" );
  } catch ( OperationException &ex ) {
    // good
  }

  try {
    objs->valstack.pop();
    FAIL( "ValueStack didn't throw exception for pop() on empty stack" );
  } catch ( OperationException &ex ) {
    // good
  }
}

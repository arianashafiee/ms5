#ifndef TABLE_H
#define TABLE_H

#include <map>
#include <string>
#include <pthread.h>

class Table {
private:
  std::string m_name; // Table name
  std::map<std::string, std::string> m_final_data;      // Committed data
  std::map<std::string, std::string> m_tentative_data; // Tentative data (uncommitted changes)
  pthread_mutex_t m_mutex; // Mutex for synchronizing access to the table

  // Copy constructor and assignment operator are prohibited
  Table(const Table &);
  Table &operator=(const Table &);

public:
  Table(const std::string &name);
  ~Table();

  std::string get_name() const { return m_name; }

  void lock();     // Acquire the table lock
  void unlock();   // Release the table lock
  bool trylock();  // Attempt to acquire the table lock

  // Note: these functions should only be called while the
  // table's lock is held!
  void set(const std::string &key, const std::string &value); // Set a key-value pair
  bool has_key(const std::string &key);                      // Check if a key exists
  std::string get(const std::string &key);                   // Get the value of a key
  void commit_changes();                                     // Commit tentative changes
  void rollback_changes();                                   // Roll back tentative changes
};

#endif // TABLE_H

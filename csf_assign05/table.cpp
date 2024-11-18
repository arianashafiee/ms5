#include <cassert>
#include "table.h"
#include "exceptions.h"
#include "guard.h"

Table::Table(const std::string &name)
    : m_name(name)
    , m_tentative_data()
    , m_final_data()
{
    pthread_mutex_init(&m_mutex, nullptr); // Initialize the mutex
}

Table::~Table()
{
    pthread_mutex_destroy(&m_mutex); // Destroy the mutex
}

void Table::lock()
{
    pthread_mutex_lock(&m_mutex); // Lock the mutex
}

void Table::unlock()
{
    pthread_mutex_unlock(&m_mutex); // Unlock the mutex
}

bool Table::trylock()
{
    return pthread_mutex_trylock(&m_mutex) == 0; // Try to lock and return success status
}

void Table::set(const std::string &key, const std::string &value)
{
    // Set a key-value pair in tentative data
    m_tentative_data[key] = value;
}

std::string Table::get(const std::string &key)
{
    // Retrieve value from tentative data if it exists, otherwise from final data
    if (m_tentative_data.count(key)) {
        return m_tentative_data[key];
    }
    if (m_final_data.count(key)) {
        return m_final_data[key];
    }
    throw OperationException("Key does not exist: " + key);
}

bool Table::has_key(const std::string &key)
{
    // Check if the key exists in either tentative or final data
    return m_tentative_data.count(key) || m_final_data.count(key);
}

void Table::commit_changes()
{
    // Commit tentative changes to final data
    for (const auto &entry : m_tentative_data) {
        if (entry.second.empty()) {
            m_final_data.erase(entry.first); // Remove key if value is empty
        } else {
            m_final_data[entry.first] = entry.second;
        }
    }
    m_tentative_data.clear(); // Clear tentative data after committing
}

void Table::rollback_changes()
{
    // Discard all tentative changes
    m_tentative_data.clear();
}

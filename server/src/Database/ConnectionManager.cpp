#include "ConnectionManager.hpp"
#include "ODBTracer.hpp"
#include <iostream>

ODBConnectionManager::ODBConnectionManager(std::size_t size, const ConnectionConfig& config) : size_(size) {
    for (std::size_t i = 0; i < size_; i++) {
        auto db = std::make_unique<odb::pgsql::database>(
                config.user, 
                config.password, 
                config.dbname, 
                config.host, 
                config.port
            );

        if (!db) {
            std::cerr << "Database connection fault!" << std::endl;
        }
        // SQLTracer tracer;
        // db->tracer(tracer);

        connection_pool_.push(
            std::move(db)
        );
    }
}

ODBConnectionManager::Connection::Connection(
    ODBConnectionManager& manager,
    std::unique_ptr<odb::pgsql::database> db) : manager_(manager), db_(std::move(db)) {
}

ODBConnectionManager::Connection::~Connection() {
    if (db_) {
        manager_.returnConnection(std::move(db_));
    }
}

void ODBConnectionManager::returnConnection(std::unique_ptr<odb::pgsql::database> db) {
    std::unique_lock lock(pool_mutex_);
    connection_pool_.push(std::move(db));
    conn_cv_.notify_one();
}

ODBConnectionManager::Connection ODBConnectionManager::acquire() {
    std::unique_lock lock(pool_mutex_);
    conn_cv_.wait(lock, [this] { return !connection_pool_.empty(); });
    auto db = std::move(connection_pool_.front());
    connection_pool_.pop();
    return Connection(*this, std::move(db));
}

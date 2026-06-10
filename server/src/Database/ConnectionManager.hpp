#ifndef CONN_MANAGER_HPP_
#define CONN_MANAGER_HPP_

#include <mutex>
#include <queue>
#include <memory>

#include <odb/connection.hxx>
#include <odb/pgsql/database.hxx>

class ODBConnectionManager {

    std::mutex pool_mutex_;
    std::queue<std::unique_ptr<odb::pgsql::database>> connection_pool_;
    std::condition_variable conn_cv_;
    std::size_t size_;

    void returnConnection(std::unique_ptr<odb::pgsql::database> db);

public: 

    struct ConnectionConfig {
        std::string user;
        std::string password;
        std::string dbname;
        std::string host;
        unsigned int port;
    };

    ODBConnectionManager(std::size_t size, const ConnectionConfig& config);

    class Connection {

        ODBConnectionManager& manager_;
        std::unique_ptr<odb::pgsql::database> db_;

        public:
            Connection(ODBConnectionManager& manager, std::unique_ptr<odb::pgsql::database> db);
            ~Connection();
            odb::pgsql::database* operator->() { return db_.get(); }
            odb::pgsql::database* operator*() { return db_.get(); }
    };

    Connection acquire();
};

#endif
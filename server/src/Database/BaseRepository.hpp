#ifndef BASE_REPOS_HPP_
#define BASE_REPOS_HPP_ 

#include <memory>
#include "ConnectionManager.hpp"

class BaseRepository {
public:
    explicit BaseRepository (std::shared_ptr<ODBConnectionManager> pool) : pool_(pool) {}
    ~BaseRepository() = default;
protected:

    template<typename F>
    auto execute(F&& f) -> decltype(f(std::declval<odb::pgsql::database&>())) {
        auto conn = pool_->acquire();
        odb::transaction t(conn->begin());
        auto result = f(**conn);
        t.commit();
        return result;
    }


    template<typename F>
    auto transaction(F&& f) -> decltype(f(std::declval<odb::transaction&>())) {
        auto conn = pool_->acquire();
        odb::transaction t(conn->begin());
        try {
            auto result = f(t);
            t.commit();
            return result;
        }
        catch (const odb::exception& e) {
            t.rollback();
            throw;
        }
    }

    std::shared_ptr<ODBConnectionManager> pool_;
};

#endif
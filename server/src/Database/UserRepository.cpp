#include "UserRepository.hpp"

#include <optional>
#include <odb/query.hxx>
#include "UserTable-odb.hxx"

namespace db {
    bool UserRepository::create(User& user) {
        return transaction([&](odb::transaction& t) -> bool {
            auto& db = t.database();

            using query = odb::query<User>;
            auto existing = db.query_one<User>(query::username == user.username());
            
            if (existing) {
                return false;
            }

            try {
                db.persist(user);
                return true;
            }
            catch (...) {
                return false;
            }
        });
    }

    std::optional<User> UserRepository::FindByName(const std::string& username) {
        return execute([&](odb::pgsql::database& t) -> std::optional<User> {
            try {
                using query = odb::query<User>;
                auto user = t.query_one<User>(query::username == username);
                if (user) {
                    return std::optional<User>(*user);
                }
                return std::nullopt;
            }
            catch (const odb::exception& e) {
                throw;
            }
        });
    }
}
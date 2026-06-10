#include "UserRepository.hpp"

#include <optional>
#include <odb/query.hxx>
#include "Models-odb.hxx"

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

        std::vector<Board> UserRepository::FindOwnedBoards(const std::string& uuid) {
            return transaction([&](odb::transaction& t) -> std::vector<Board> {
                auto& db = t.database();

                auto user = db.find<User>(uuid);
                
                if (!user) {
                    return {};
                }

                using board_query = odb::query<Board>;
                auto result = db.query<Board>(board_query::owner->uuid == user->uuid());

                std::vector<Board> out;
                for (Board& board : result) {
                    out.push_back(std::move(board));
                }

                return out;
            });
        }
}
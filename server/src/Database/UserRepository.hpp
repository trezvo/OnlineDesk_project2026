#ifndef USER_REPOS_HPP_
#define USER_REPOS_HPP_

#include "BaseRepository.hpp"
#include "Models.hxx"

#include <optional>

namespace db {
    class UserRepository : public BaseRepository {

    public:
        using BaseRepository::BaseRepository;

        bool create(User& user);
        std::optional<User> FindByName(const std::string& username);
        std::vector<Board> FindOwnedBoards(const std::string& uuid);
    };
}


#endif

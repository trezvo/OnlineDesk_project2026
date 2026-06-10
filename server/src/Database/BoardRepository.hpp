#ifndef BOARD_REPOS_HPP_
#define BOARD_REPOS_HPP_

#include "BaseRepository.hpp"
#include "Models.hxx"
#include <string>
#include <variant>

namespace db {
    class BoardRepository : public BaseRepository
    {
    public:

        enum class CODE_ID {
            OK = 200,
            USER_NOT_FOUND = 404,
            BOARD_NOT_FOUND = 405,
            BOARD_ALREADY_EXISTS = 406,
            INTERNAL_ERROR = 505
        };

        using BaseRepository::BaseRepository;
        using findByIdResult = std::variant<Board, CODE_ID>;

        CODE_ID create(Board& board, const std::string& owner);
        CODE_ID remove(uint64_t board_id);
        CODE_ID update(Board& board);
        findByIdResult findById(uint64_t board_id);
    };
}


#endif

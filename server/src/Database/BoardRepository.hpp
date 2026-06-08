#ifndef BOARD_REPOS_HPP_
#define BOARD_REPOS_HPP_

#include "BaseRepository.hpp"
#include "Models.hxx"
#include <string>

namespace db {
    class BoardRepository : public BaseRepository
    {
    public:
        using BaseRepository::BaseRepository;

        bool create(Board& board, std::string owner);
        void remove(unsigned long id);
        void update(const std::string& name);
    };
}


#endif

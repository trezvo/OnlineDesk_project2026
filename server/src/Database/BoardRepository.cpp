#include "BoardRepository.hpp"
#include "Models-odb.hxx"
#include <odb/query.hxx>

namespace db {

bool BoardRepository::create(Board& board, std::string& owner) {
    return transaction([&] (odb::transaction& t) -> bool {
        auto& db = t.database();
        
        auto existing_user = db.query_one<User>(odb::query<User>::uuid == owner.uuid());
        
        if (!existing_user) {
            return false;
        }

        auto existing_board = db.query_one<Board>(
            odb::query<Borad>::owner == owner.uuid() && odb::query<Board>::name == board.name());

        if (existing_board) {
            return false;
        } 

        try {
            db.persist(board);
            return true;
        }
        catch (...) {
            return false;
        }
    });
}

void BoardRepository::remove(Board& board) {

}

}
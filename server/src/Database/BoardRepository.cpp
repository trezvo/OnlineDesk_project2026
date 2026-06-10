#include "BoardRepository.hpp"
#include "Models-odb.hxx"
#include <odb/query.hxx>
#include <iostream>
#include <memory>

namespace db {

BoardRepository::CODE_ID BoardRepository::create(Board& board, const std::string& owner) {
    return transaction([&] (odb::transaction& t) -> CODE_ID {
        auto& db = t.database();
        
        auto user = db.find<User>(owner);
        
        if (!user) {
            return CODE_ID::USER_NOT_FOUND;
        }

        board.owner(std::shared_ptr<User>(user));

        try {
            db.persist(board);
            return CODE_ID::OK;
        }
        catch (const odb::exception& e) {
            std::cout << e.what() << std::endl;
            return CODE_ID::INTERNAL_ERROR;
        }
    });
}

BoardRepository::CODE_ID BoardRepository::remove(uint64_t board_id) {
    return transaction([&] (odb::transaction& t) -> CODE_ID {
        auto& db = t.database();
        try
        {
            db.erase<Board>(board_id);
            return CODE_ID::OK;
        }
        catch(const odb::object_not_persistent&)
        {
            return CODE_ID::BOARD_NOT_FOUND;
        }
    });
}

BoardRepository::CODE_ID BoardRepository::update(Board& board) {
    return transaction([&] (odb::transaction& t) {
        auto& db = t.database();

        try {
            db.update(board);
            return CODE_ID::OK;
        }
        catch (const odb::object_not_persistent&) {
            return CODE_ID::BOARD_NOT_FOUND;
        }
        catch (const odb::exception&) {
            return CODE_ID::INTERNAL_ERROR;
        }
    });
}

BoardRepository::findByIdResult BoardRepository::findById(uint64_t board_id) {
    return transaction([&] (odb::transaction& t) -> findByIdResult {
        auto& db = t.database();

        auto board = db.find<Board>(board_id);

        if (!board) {
            return CODE_ID::BOARD_NOT_FOUND;
        }

        return *board;
    });
}

}
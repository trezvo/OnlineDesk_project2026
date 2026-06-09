#ifndef WIDGET_REPOS_HPP_
#define WIDGET_REPOS_HPP_

#include "BaseRepository.hpp"
#include "Models.hxx"
#include <optional>

namespace db {

    class WidgetRepository : public BaseRepository {
    
    public:

        enum class CODE_ID {
            OK = 200,
            WIDGET_ALREADY_EXISTS = 404,
            BOARD_ALREADY_EXISTS = 405,
            BOARD_NOT_FOUND = 406,
            WIDGET_NOT_FOUND = 407,
            INTERNAL_ERROR = 408
        };

        using BaseRepository::BaseRepository;

        CODE_ID create(Widget& widget);
        CODE_ID create(Widget& widget, uint64_t board_id);
        CODE_ID remove(Widget& widget);
        CODE_ID remove(uint64_t widget_id);
        CODE_ID update(Widget& widget);
        CODE_ID update(Widget& widget, uint64_t board_id);
        uint64_t createSnapshot(uint64_t old_board, uint64_t new_board);
        std::optional<Widget> findById(uint64_t widget_id);
        std::vector<Widget> findAllOnBoard(unsigned long long board_id);
    };

}

#endif
#ifndef WIDGET_REPOS_HPP_
#define WIDGET_REPOS_HPP_

#include "BaseRepository.hpp"
#include "Models.hxx"
#include <optional>
#include <random>
#include <chrono>

namespace db {

    class WidgetRepository : public BaseRepository {
    
        inline static uint64_t gen64_() {
            static std::mt19937_64 gen_(std::chrono::system_clock::now().time_since_epoch().count());
            return gen_();
        }

    public:

        enum class CODE_ID {
            OK = 200,
            WIDGET_ALREADY_EXISTS = 404,
            BOARD_ALREADY_EXISTS = 405,
            BOARD_NOT_FOUND = 406,
            WIDGET_NOT_FOUND = 407,
            USER_NOT_FOUND = 408,
            INTERNAL_ERROR = 409
        };

        using BaseRepository::BaseRepository;

        CODE_ID create(Widget& widget);
        CODE_ID create(Widget& widget, uint64_t board_id);
        CODE_ID remove(Widget& widget);
        CODE_ID remove(uint64_t widget_id);
        CODE_ID update(Widget& widget);
        CODE_ID update(Widget& widget, uint64_t board_id);
        uint64_t createSnapshot(uint64_t old_board, const std::string& owner_uuid);
        std::optional<Widget> findById(uint64_t widget_id);
        std::vector<Widget> findAllOnBoard(unsigned long long board_id);
    };

}

#endif
#include "WidgetsDB.hpp"

void WidgetDataBase::Post(uint64_t widget_id, WidgetsPost body) {
    std::lock_guard<std::mutex> lock(talbe_mutex_);
    widgets_talbe_[widget_id] = {body.board_id, body.x, body.y};
}

void WidgetDataBase::Update(uint64_t widget_id, WidgetsUpdate body) {
    std::lock_guard<std::mutex> lock(talbe_mutex_);
    WidgetsTableWrite& write = widgets_talbe_[widget_id];
    write.coord_x = body.x;
    write.coord_y = body.y;
}

void WidgetDataBase::Delete(uint64_t widget_id) {
    std::lock_guard<std::mutex> lock(talbe_mutex_);
    widgets_talbe_.erase(widget_id);
}

WidgetsRead WidgetDataBase::Get(uint64_t widget_id) {
    
    WidgetsTableWrite response;
    {   
        std::lock_guard<std::mutex> lock(talbe_mutex_);
        response = widgets_talbe_[widget_id];
    }

    return {
        widget_id,
        response.board_id,
        response.coord_x,
        response.coord_y
    };
}

std::vector<WidgetsRead> WidgetDataBase::SelectFromBoard(uint64_t board_id) {
    std::lock_guard<std::mutex> lock(talbe_mutex_);

    std::vector<WidgetsRead> response;

    for (auto [widget_id, read] : widgets_talbe_) {
        if (read.board_id != board_id) {
            continue;
        }

        response.emplace_back(
            widget_id,
            board_id,
            read.coord_x,
            read.coord_y
        );
    }

    return response;
}
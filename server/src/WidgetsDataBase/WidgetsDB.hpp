#pragma once

#include "unordered_map"
#include "vector"
#include "mutex"
#include "functional"

struct WidgetsPost {
    uint64_t board_id;
    int x;
    int y;
};

struct WidgetsRead {
    uint64_t widget_id;
    uint64_t board_id;
    int x;
    int y;
};

struct WidgetsUpdate {
    int x, y;
};

struct WidgetsTableWrite {
    
    uint64_t board_id;
    int coord_x;
    int coord_y;
};

class WidgetDataBase {

    std::mutex talbe_mutex_;
    std::unordered_map<uint64_t, WidgetsTableWrite> widgets_talbe_;

public:

    void Post(uint64_t widget_id, WidgetsPost body);
    void Update(uint64_t widget_id, WidgetsUpdate body);
    WidgetsRead Get(uint64_t widget_id);
    std::vector<WidgetsRead> SelectFromBoard(uint64_t board_id);
    void Delete(uint64_t widget_id);
    void DeleteByBoardId(uint64_t board_id);

};
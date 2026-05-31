#pragma once

#include "unordered_map"
#include "vector"
#include "mutex"
#include "functional"
#include "string"

struct WidgetsPost {
    uint64_t board_id;
    int x;
    int y;
    std::string content;
};

struct WidgetsRead {
    uint64_t widget_id;
    uint64_t board_id;
    int x;
    int y;
    std::string content;
};

struct WidgetsUpdate {
    int x, y;
    std::string content;
};

struct WidgetsTableWrite {
    uint64_t board_id;
    int coord_x;
    int coord_y;
    std::string content;
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
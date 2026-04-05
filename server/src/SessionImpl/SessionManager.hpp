#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <random>
#include <chrono>
#include <vector>
#include <unordered_map>
#include "BoardImpl/BoardServiceImplFwd.hpp"
#include "WidgetsDataBase/WidgetsDB.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"

namespace board_module {

namespace contracts = online_desk::board;


class SessionManager {
    
    std::mt19937 create_rand_64_ = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
    std::mutex sessions_table_mutex_;
    std::unordered_map<uint64_t, SessionInstance*> sessions_table_;
    WidgetDataBase widgets_db_;

public:

    uint64_t MakeBoardSnapshot(uint64_t board_id);
    std::vector<WidgetsRead> GetBoardsWidgets(uint64_t board_id);
    WidgetsRead GetWidget(uint64_t widget_id);
    void AddWidget(uint64_t widget_id, WidgetsPost body);
    void UpdateWidget(uint64_t widget_id, WidgetsUpdate body);
    void DeleteWidget(uint64_t widget_id);
    void CloseSession(uint64_t board_id);
    SessionInstance* JoinToSession(SessionReactor *member, uint64_t board_id);
};

}  // namespace board_module

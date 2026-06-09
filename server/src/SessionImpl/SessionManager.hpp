#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <random>
#include <chrono>
#include <vector>
#include <unordered_map>
#include "BoardImpl/BoardServiceImplFwd.hpp"
#include "WidgetsDataBase/WidgetsDB.hpp"
#include "Database/WidgetRepository.hpp"
#include "Database/BoardRepository.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"
#include "Database/Models.hxx"

namespace board_module {

namespace contracts = online_desk::board;

class SessionManager {
    
    std::mt19937 create_rand_64_ = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
    std::mutex sessions_table_mutex_;
    std::unordered_map<uint64_t, SessionInstance*> sessions_table_;
    std::shared_ptr<db::WidgetRepository> widget_table_;
    std::shared_ptr<db::BoardRepository> board_table_; 

public:

    SessionManager(
        std::shared_ptr<db::WidgetRepository> widget_table,
        std::shared_ptr<db::BoardRepository> board_table) 
    :   widget_table_(widget_table), 
        board_table_(board_table) {}

    uint64_t MakeBoardSnapshot(uint64_t old_board_id, uint64_t new_board_id);
    std::vector<db::Widget> GetBoardsWidgets(uint64_t board_id);
    db::Widget GetWidget(uint64_t widget_id);
    void AddWidget(db::Widget widget, uint64_t board_id);
    void UpdateWidget(db::Widget widget, uint64_t board_id);
    void DeleteWidget(uint64_t widget_id);
    void CloseSession(uint64_t board_id);
    void BroadcastToSession(uint64_t board_id, const contracts::BoardUpdate& message);
    SessionInstance* JoinToSession(SessionReactor *member, uint64_t board_id);
    void ShutdownAllInSession(uint64_t board_id);
};

}  // namespace board_module

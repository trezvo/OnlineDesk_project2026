#include "SessionManager.hpp"
#include "SessionInstance.hpp"

namespace board_module {

namespace contracts = online_desk::board;

SessionInstance* 
SessionManager::JoinToSession(SessionReactor* member, uint64_t board_id) {
    std::lock_guard<std::mutex> lock(sessions_table_mutex_);

    if (!sessions_table_.contains(board_id)) {
        sessions_table_[board_id] = new SessionInstance(*this, board_id);
    }

    return sessions_table_[board_id]->JoinToSession(member);
}

void SessionManager::CloseSession(uint64_t board_id) {
    std::lock_guard<std::mutex> lock(sessions_table_mutex_);

    if (sessions_table_.contains(board_id)) {
        sessions_table_.erase(board_id);
    }
}

uint64_t SessionManager::MakeBoardSnapshot(uint64_t board_id) {
    uint64_t new_board_id = create_rand_64_();

    auto widgets_on_board = widgets_db_.SelectFromBoard(board_id);

    for (WidgetsRead read : widgets_on_board) {
        widgets_db_.Post(
            create_rand_64_(), { new_board_id, read.x, read.y });
    }

    return new_board_id;
}


std::vector<WidgetsRead> SessionManager::GetBoardsWidgets(uint64_t board_id) {
    return widgets_db_.SelectFromBoard(board_id);
}

WidgetsRead SessionManager::GetWidget(uint64_t widget_id) {
    return widgets_db_.Get(widget_id);
}

void SessionManager::AddWidget(uint64_t widget_id, WidgetsPost body) {
    widgets_db_.Post(widget_id, std::move(body));
}

void SessionManager::UpdateWidget(uint64_t widget_id, WidgetsUpdate body) {
    widgets_db_.Update(widget_id, std::move(body));
}

void SessionManager::DeleteWidget(uint64_t widget_id) {
    widgets_db_.Delete(widget_id);   
}

void SessionManager::DeleteBoardWidgets(uint64_t board_id) {
    widgets_db_.DeleteByBoardId(board_id);
}

void SessionManager::BroadcastToSession(uint64_t board_id, const contracts::BoardUpdate& message) {
    std::lock_guard<std::mutex> lock(sessions_table_mutex_);
    if (sessions_table_.contains(board_id)) {
        sessions_table_[board_id]->BroadcastToAll(message);
    }
}

}  // namespace board_module

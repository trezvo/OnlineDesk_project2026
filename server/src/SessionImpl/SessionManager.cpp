#include "SessionManager.hpp"
#include "SessionInstance.hpp"
#include <memory>
#include <iostream>

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

uint64_t SessionManager::MakeBoardSnapshot(uint64_t old_board_id, const std::string& user_uuid) {
    try {
        return widget_table_->createSnapshot(old_board_id, user_uuid);
    }
    catch (const odb::exception& e) {
        std::cout << "create snapshot: " << e.what() << std::endl;
        return 0;
    }
    catch (...) {
        std::cout << "create snapshot: board not found" << std::endl;
        return 0;
    }
}


std::vector<db::Widget> SessionManager::GetBoardsWidgets(uint64_t board_id) {
    return widget_table_->findAllOnBoard(board_id);
}

db::Widget SessionManager::GetWidget(uint64_t widget_id) {
    auto widget_query =  widget_table_->findById(widget_id);
    
    if (!widget_query.has_value()) {
        return {};
    }

    return widget_query.value();
}

void SessionManager::AddWidget(db::Widget widget, uint64_t board_id) {
    try {widget_table_->create(widget, board_id); }
    catch (const odb::exception& e) { 
        std::cout << e.what() << std::endl;
    }

    
}

void SessionManager::UpdateWidget(db::Widget widget, uint64_t board_id) {
    widget_table_->update(widget, board_id);
}

void SessionManager::DeleteWidget(uint64_t widget_id) {
    widget_table_->remove(widget_id);   
}

void SessionManager::BroadcastToSession(uint64_t board_id, const contracts::BoardUpdate& message) {
    std::lock_guard<std::mutex> lock(sessions_table_mutex_);
    if (sessions_table_.contains(board_id)) {
        sessions_table_[board_id]->BroadcastToAll(message);
    }
}

void SessionManager::ShutdownAllInSession(uint64_t board_id) {
    std::lock_guard<std::mutex> lock(sessions_table_mutex_);
    if (sessions_table_.contains(board_id)) {
        sessions_table_[board_id]->ShutdownAll();
    }
}

}  // namespace board_module

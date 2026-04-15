#include "SessionInstance.hpp"
#include "SessionManager.hpp"
#include "SessionReactor.hpp"
#include <vector>
#include "WidgetsDataBase/WidgetsDB.hpp"
#include <iostream>

namespace board_module {

SessionInstance::SessionInstance(SessionManager& manager, uint64_t board_id) 
    : manager_(manager)
    , board_id_(board_id) {
    std::cout << "request to initiate board id=" << board_id << std::endl;
    std::vector<WidgetsRead> widgets = manager_.GetBoardsWidgets(board_id_);

    for (const WidgetsRead& read : widgets) {
        widgets_storage_.insert(read.widget_id);
    }

    std::cout << "created session instance of " << board_id << ", widgets amount=" << widgets.size() << std::endl;
}

void SessionInstance::CloseMemberConnection(SessionReactor *member) {
    session_members_.erase(member);

    if (session_members_.empty()) {
        manager_.CloseSession(board_id_);
        delete this;
    }
}

SessionInstance* SessionInstance::JoinToSession(SessionReactor* member) {
    std::lock_guard<std::mutex> lock(board_edit_mutex_);
    session_members_.insert(member);
    return this;
}

void SessionInstance::BroadcastToAll(const contracts::BoardUpdate& message) {
    std::lock_guard<std::mutex> lock(board_edit_mutex_);
    for (auto member : session_members_) {
        member->ProcessMessage(message);
    }
}

}
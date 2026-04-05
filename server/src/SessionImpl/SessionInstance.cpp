#include "SessionInstance.hpp"
#include "SessionManager.hpp"
#include <vector>
#include "WidgetsDataBase/WidgetsDB.hpp"

namespace board_module {

SessionInstance::SessionInstance(SessionManager& manager, uint64_t board_id) 
    : manager_(manager)
    , board_id_(board_id) {
    std::vector<WidgetsRead> widgets = manager_.GetBoardsWidgets(board_id_);

    for (const WidgetsRead& read : widgets) {
        widgets_storage_.insert(read.widget_id);
    }
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

}
#include "SessionImpl.hpp"
#include <memory>
#include <mutex>
#include <set>
#include <utility>

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

SessionInstance::SessionInstance(SessionManager& manager, uint64_t board_id) 
    : manager_(manager)
    , board_id_(board_id) {
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

SessionReactor::SessionReactor(grpc::experimental::CallbackServerContext* context, SessionManager& manager) 
    : context_(context)
    , manager_(manager) {

    const auto& metadata = context->client_metadata();

    auto board_id_it = metadata.find("custom-board-id");
    uint64_t board_id = std::stoi(std::string(board_id_it->second.data()));

    {
        session_instance_ = manager_.JoinToSession(this, board_id);

        std::lock_guard<std::mutex> lock(session_instance_->board_edit_mutex_);

        contracts::BoardUpdate response;

        for (const auto& [id, widget] : session_instance_->widgets_storage_) {
            response.set_action_type(online_desk::board::CREATE);
            response.set_widget_id(id);
            response.set_user_token(0);

            contracts::WidgetInfo* diff = response.mutable_update_data();
            diff->set_coord_x(widget.x);
            diff->set_coord_y(widget.y);
            ProcessMessage(response);
        }

    }

    
    StartRead(&request_);
}

void SessionReactor::Broadcast(const contracts::BoardUpdate &request) {

    std::lock_guard<std::mutex> lock(session_instance_->board_edit_mutex_);

    online_desk::board::ActionType action = request.action_type();

    switch (action) {
        case (online_desk::board::EXIT): {
            Shutdown();
        } break;
        default: {
        } break;
    }

    uint64_t widget_id = request.widget_id();

    const online_desk::board::WidgetInfo& update_info = request.update_data();

    if (action == online_desk::board::DELETE) {
        if (!session_instance_->widgets_storage_.contains(widget_id)) {
            return;
        }
        session_instance_->widgets_storage_.erase(widget_id);
    } 
    else {

        int coord_x = update_info.coord_x();
        int coord_y = update_info.coord_y();

        session_instance_->widgets_storage_[widget_id] = {coord_x, coord_y};
    }

    for (auto member : session_instance_->session_members_) {
        if (member == this) {
            continue;
        }

        member->ProcessMessage(request);
    }
}

void SessionReactor::Shutdown() {
    if (!is_alive.exchange(false)) {
        return;
    }

    Finish(grpc::Status::OK);
}

void SessionReactor::ProcessMessage(const contracts::BoardUpdate &msg) {
    if (!is_alive) {
        return;
    }

    std::lock_guard<std::mutex> lock(write_mutex_);

    auto msg_copy = std::make_unique<contracts::BoardUpdate>(msg);
    msg_copy->set_user_token(0);
    write_queue_.push_back(std::move(msg_copy));

    StartWrite(write_queue_.back().get());
}

void SessionReactor::OnWriteDone(bool ok) {
    
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (!write_queue_.empty()) {
            write_queue_.pop_front();
        }
    }

    if (!ok) {
        Shutdown();
    }
}

void SessionReactor::OnReadDone(bool ok) {
    if (!ok) {
        Shutdown();
        return;
    }

    Broadcast(request_);
    StartRead(&request_);
}

void SessionReactor::OnCancel() {
    Shutdown();
}

void SessionReactor::OnDone() {
    {
        std::lock_guard<std::mutex> lock(session_instance_->board_edit_mutex_);
        session_instance_->CloseMemberConnection(this);
    }

    delete this;
}

}  // namespace board_module

#include "SessionReactor.hpp"
#include "SessionInstance.hpp"
#include "SessionManager.hpp"

namespace board_module {

SessionReactor::SessionReactor(grpc::CallbackServerContext* context, SessionManager& manager) 
    : context_(context)
      , manager_(manager)
      , is_alive(true)
      , is_writing_(false) {

    const auto& metadata = context->client_metadata();

    auto board_id_it = metadata.find("custom-board-id");
    uint64_t board_id = std::stoull(std::string(board_id_it->second.data()));

    {
        session_instance_ = manager_.JoinToSession(this, board_id);

        std::lock_guard<std::mutex> lock(session_instance_->board_edit_mutex_);

        contracts::BoardUpdate response;

        for (uint64_t widget_id : session_instance_->widgets_storage_) {
            const db::Widget widget = manager_.GetWidget(widget_id);

            response.set_action_type(online_desk::board::CREATE);
            response.set_widget_id(widget.id());
            response.set_user_token(0);

            contracts::WidgetInfo* diff = response.mutable_update_data();
            diff->set_coord_x(widget.x());
            diff->set_coord_y(widget.y());
            diff->set_content(widget.content());
            ProcessMessage(response);
        }

    }

    // std::cout << "session reactor succesfuly init for board id=" << board_id << std::endl;
    
    StartRead(&request_);
}

void SessionReactor::Broadcast(const contracts::BoardUpdate &request) {

    std::lock_guard<std::mutex> lock(session_instance_->board_edit_mutex_);

    online_desk::board::ActionType action = request.action_type();

    // std::cout << "content: " << request.update_data().content() << std::endl;

    switch (action) {
        case (online_desk::board::EXIT): {
            Shutdown();
            return;
        } break;
        case (online_desk::board::CREATE): {
            manager_.AddWidget(db::Widget(
                request.widget_id(),
                request.update_data().coord_x(), 
                request.update_data().coord_y(),
                request.update_data().content(),
                nullptr), session_instance_->board_id_
            );
            session_instance_->widgets_storage_.insert(request.widget_id());
        } break;
        case (online_desk::board::UPDATE): {
            if (!session_instance_->widgets_storage_.contains(request.widget_id())) {
                return;
            }

            manager_.UpdateWidget(db::Widget(
                request.widget_id(),
                request.update_data().coord_x(),
                request.update_data().coord_y(),
                request.update_data().content(),
                nullptr), session_instance_->board_id_
            );
        } break;
        case (online_desk::board::DELETE): {
            if (!session_instance_->widgets_storage_.contains(request.widget_id())) {
                return;
            }

            manager_.DeleteWidget(request.widget_id());
            session_instance_->widgets_storage_.erase(request.widget_id());
        } break;
        default: {
            return;
        } break;
    }

    uint64_t widget_id = request.widget_id();

    // std::cout << "income update: id=" << widget_id << std::endl;

    const online_desk::board::WidgetInfo& update_info = request.update_data();

    contracts::BoardUpdate message;
    message.set_action_type(request.action_type());
    message.set_user_token(request.user_token());
    message.set_widget_id(request.widget_id());

    contracts::WidgetInfo* info = message.mutable_update_data();
    info->set_coord_x(update_info.coord_x());
    info->set_coord_y(update_info.coord_y());
    info->set_content(update_info.content());

    for (auto member : session_instance_->session_members_) {
        if (member == this) {
            continue;
        }

        member->ProcessMessage(message);
    }
}

void SessionReactor::Shutdown() {
    if (!is_alive.exchange(false)) {
        return;
    }
    Finish(grpc::Status::OK);
}

void SessionReactor::ProcessMessage(contracts::BoardUpdate msg) {
    if (!is_alive) {
        return;
    }
    if (msg.action_type() == online_desk::board::BOARD_DELETED) {
        pending_shutdown_ = true;
    }
        
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_queue_.push_back(std::move(msg));
    }

    ProcessQueue();
}

void SessionReactor::ProcessQueue() {
    if (!is_alive || is_writing_.exchange(true)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(write_mutex_);

        if (write_queue_.empty()) {
            is_writing_ = false;
            return;
        }

        request_ = std::move(write_queue_.front());
        write_queue_.pop_front();
    }

    StartWrite(&request_);
}

void SessionReactor::OnWriteDone(bool ok) {
    if (!is_alive) {
        return;
    }

    is_writing_ = false;

    if (!ok) {
        Shutdown();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (write_queue_.empty() && pending_shutdown_) {
            Shutdown(); 
            return;
            }
    }

    ProcessQueue();
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

void SessionReactor::BroadcastBoardDeleted() {
    contracts::BoardUpdate message;
    message.set_action_type(online_desk::board::BOARD_DELETED);
    message.set_widget_id(0);

    for (auto member : session_instance_->session_members_) {
        member->ProcessMessage(message);
    }
}

}

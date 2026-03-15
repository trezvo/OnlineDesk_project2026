#include "SessionReactor.hpp"
#include <utility>
#include <memory>

SessionReactor::SessionReactor(BoardService::Stub* stub_, std::unique_ptr<grpc::ClientContext> context, BoardScreen& board)
    : is_running_(true)
    , screen_instance_(board)
    , is_writing_(false) {
        
    stub_->experimental_async()->SubscribeBoard(context.get(), this);

    StartCall();
    StartRead(&read_buffer_);
}

void SessionReactor::AddUpdate(BoardUpdate request) {
    if (!is_running_) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(write_queue_mutex_);
        write_queue_.push(std::move(request));
    }

    ProcessQueue();
}

void SessionReactor::ProcessQueue() {
    if (!is_running_ || is_writing_.exchange(true)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(write_queue_mutex_);
        if (write_queue_.empty()) {
            is_writing_ = false;
            return;
        }
        write_buffer_ = std::move(write_queue_.front());
        write_queue_.pop();
    }

    StartWrite(&write_buffer_);
}

void SessionReactor::OnWriteDone(bool ok) {
    if (!is_running_) {
        return;
    }

    is_writing_ = false;

    if (!ok) {
        Shutdown();
        return;
    }

    ProcessQueue();
}

void SessionReactor::OnReadDone(bool ok) {
    if (!is_running_) {
        return;
    }
    
    if (!ok) {
        Shutdown();
        return;
    }

    screen_instance_.UpdateBoard(read_buffer_);
    StartRead(&read_buffer_);
}

void SessionReactor::OnDone(const grpc::Status& status) {
    delete this;
}

void SessionReactor::Shutdown() {
    if (!is_running_.exchange(false)) {
        return;
    }

    StartWritesDone();
}
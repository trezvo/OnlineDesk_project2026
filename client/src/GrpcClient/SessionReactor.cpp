#include "SessionReactor.hpp"
#include <utility>
#include <memory>
#include <iostream>

SessionReactor::SessionReactor(BoardService::Stub* stub_, std::unique_ptr<grpc::ClientContext> context, BoardWorkerInterface& worker)
    : is_running_(true)
    , board_worker_(worker)
    , is_writing_(false) {
        
    stub_->async()->SubscribeBoard(context.get(), this);

    StartCall();
    StartRead(&read_buffer_);
}

void SessionReactor::AddUpdate(BoardUpdate request) {
    if (!is_running_) {
        return;
    }

    std::cout << "entered 'AddUpdate'..." << std::endl;
    {
        std::lock_guard<std::mutex> lock(write_queue_mutex_);
        write_queue_.push(std::move(request));
    }
    ProcessQueue();

    std::cout << "'AddUpdate' freed!" << std::endl;
}

void SessionReactor::ProcessQueue() {
    if (!is_running_ || is_writing_.exchange(true)) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(write_queue_mutex_);

        if (write_queue_.empty()) {
            is_writing_ = false;
            return;
        }

        write_buffer_ = std::move(write_queue_.front());
        write_queue_.pop();
    }

    write_buffer_.mutable_update_data()->clear_content();
    StartWrite(&write_buffer_);
}

void SessionReactor::OnWriteDone(bool ok) {
    if (!is_running_ or !ok) {
        return;
    }

    std::cout << "outcome bidi update" << std::endl;
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

    std::cout << "income bidi update" << std::endl;

    board_worker_.addUpdate(std::move(read_buffer_));
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
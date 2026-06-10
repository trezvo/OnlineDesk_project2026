#include "SessionReactor.hpp"
#include <utility>
#include <memory>
#include <iostream>

SessionReactor::SessionReactor(BoardService::Stub* stub, std::unique_ptr<grpc::ClientContext> context, BoardWorkerInterface* worker)
    : is_running_(true)
    , board_worker_(worker)
    , is_writing_(false)  {
        
    stub->async()->SubscribeBoard(context.get(), this);

    StartCall();
    StartRead(read_buffer_.get());
}

void SessionReactor::AddUpdate(BoardUpdate request) {
    if (!is_running_) {
        return;
    }

    // std::cout << "entered 'AddUpdate'..." << std::endl;
    {
        std::lock_guard<std::mutex> lock(write_queue_mutex_);
        write_queue_.push(std::make_unique<BoardUpdate>(std::move(request)));
    }
    ProcessQueue();

    // std::cout << "'AddUpdate' freed!" << std::endl;
}

void SessionReactor::ProcessQueue() {
    std::unique_lock<std::mutex> lock(write_queue_mutex_);

    if (!is_running_ || is_writing_ || write_queue_.empty()) {
        return;
    }

    is_writing_ = true; 

    write_buffer_ = std::move(write_queue_.front());
    write_queue_.pop();

    BoardUpdate* buffer_ptr = write_buffer_.get();

    lock.unlock();

    StartWrite(buffer_ptr);
}

void SessionReactor::OnWriteDone(bool ok) {
    std::unique_lock<std::mutex> lock(write_queue_mutex_);
    
    if (!is_running_) {
        return;
    }
    
    if (!ok) {
        Shutdown();
        return;
    }
    
    write_buffer_.reset();
    is_writing_ = false;
    
    if (!write_queue_.empty() && is_running_) {
        is_writing_ = true;
        write_buffer_ = std::move(write_queue_.front());
        write_queue_.pop();
        
        BoardUpdate* buffer_ptr = write_buffer_.get();
        
        lock.unlock();
        
        StartWrite(buffer_ptr);
    }
}

void SessionReactor::OnReadDone(bool ok) {
    if (!is_running_) {
        return;
    }
    
    if (!ok) {
        Shutdown();
        return;
    }

    if (board_worker_) {
        BoardUpdate tmp = *read_buffer_.release();
        board_worker_->addUpdate(std::move(tmp));
        read_buffer_ = std::make_unique<BoardUpdate>();
    }

    StartRead(read_buffer_.get());
}

void SessionReactor::OnDone(const grpc::Status& status) {
    if (board_worker_) {
        board_worker_->Shutdown();
    }
    delete this;
}

void SessionReactor::Shutdown() {
    if (!is_running_.exchange(false)) {
        return;
    }

    StartWritesDone();
}

void SessionReactor::DetachWorker() {
    board_worker_ = nullptr;
}
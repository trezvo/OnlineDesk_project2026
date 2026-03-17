#include "BoardWorker.hpp"
#include "GrpcBoardClient.hpp"
#include "SessionReactorInterface.hpp"
#include <iostream>

BoardWorker::BoardWorker(std::shared_ptr<GrpcBoardClient> grpc_client, uint64_t board_id) 
    : is_running_(true) {
    stream_ = grpc_client->connectToBoard(*this, board_id);
}

void BoardWorker::runWorking() {
    std::cout << "board worked is running" << std::endl;

    while (is_running_) {
        std::unique_lock<std::mutex> lock(income_update_mutex_);
        income_update_cv_.wait(lock, [this] {
            return !is_running_ || !income_updates_queue_.empty();
        });

        std::cout << "loop msg" << std::endl;

        if (!is_running_ || income_updates_queue_.empty()) {
            continue;
        }

        Action action = income_updates_queue_.front().first;
        auto update = std::move(income_updates_queue_.front().second);
        income_updates_queue_.pop();

        switch (action) {
            case (BoardWorker::Action::READ): {
                emit printUpdate(std::move(update));
            } break;
            case (BoardWorker::Action::WRITE): {
                stream_->AddUpdate(std::move(update));
            } break;
            default: {
            } break;
        }
    }
}

void BoardWorker::sendSessionUpdate(online_desk::board::BoardUpdate update) {
    std::cout << "worker handled update, w_id=" << update.widget_id() << std::endl;
    {
        std::lock_guard<std::mutex> lock(income_update_mutex_);
        income_updates_queue_.push({BoardWorker::Action::WRITE, std::move(update)});
        income_update_cv_.notify_one();
    }
}

void BoardWorker::Shutdown() {
    if (!is_running_.exchange(false)) {
        return;
    }

    stream_->Shutdown();
}

void BoardWorker::addUpdate(online_desk::board::BoardUpdate update) {
    {
        std::lock_guard<std::mutex> lock(income_update_mutex_);
        income_updates_queue_.push({BoardWorker::Action::READ, std::move(update)});
        income_update_cv_.notify_one();
    }
}

BoardWorker::~BoardWorker() {
    Shutdown();
}
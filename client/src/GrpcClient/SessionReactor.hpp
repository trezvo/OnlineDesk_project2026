#pragma once

#include "BoardScreen/BoardWorkerInterface.hpp"
#include "SessionReactorInterface.hpp"
#include <grpcpp/grpcpp.h>
#include "board.grpc.pb.h"
#include "board.pb.h"
#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>

using namespace online_desk::board;

class SessionReactor : public SessionReactorInterface {

    std::unique_ptr<grpc::ClientContext> context_;

    std::atomic<bool> is_running_;
    BoardWorkerInterface* board_worker_;
    std::unique_ptr<BoardUpdate> read_buffer_ = std::make_unique<BoardUpdate>();
    std::unique_ptr<BoardUpdate> write_buffer_ = nullptr;
    
    std::atomic<bool> is_writing_;
    std::mutex write_queue_mutex_;
    std::queue<std::unique_ptr<BoardUpdate>> write_queue_;

    void ProcessQueue();

public:

    explicit SessionReactor(BoardService::Stub* stub, std::unique_ptr<grpc::ClientContext> context, BoardWorkerInterface* worker); 
    void DetachWorker();    
    void AddUpdate(BoardUpdate request) override;
    void OnWriteDone(bool ok) override;
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status& status) override;

    void Shutdown() override;
    ~SessionReactor() override {
        Shutdown();
    };
};

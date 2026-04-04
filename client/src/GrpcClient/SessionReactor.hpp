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

    std::atomic<bool> is_running_;
    BoardWorkerInterface& board_worker_;
    BoardUpdate read_buffer_;
    BoardUpdate write_buffer_;
    
    std::atomic<bool> is_writing_;
    std::condition_variable writing_cv_;
    std::mutex write_queue_mutex_;
    std::queue<BoardUpdate> write_queue_;

    void ProcessQueue();
    ~SessionReactor() override = default;

public:

    explicit SessionReactor(BoardService::Stub* stub_, std::unique_ptr<grpc::ClientContext> context, BoardWorkerInterface& worker); 

    void AddUpdate(BoardUpdate request) override;

    void OnWriteDone(bool ok) override;
    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status& status) override;

    void Shutdown() override;

};

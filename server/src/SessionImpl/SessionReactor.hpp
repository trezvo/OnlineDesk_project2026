#pragma once 

#include <list>
#include <atomic>
#include <mutex>
#include <grpc++/grpc++.h>
#include "board.grpc.pb.h"
#include "board.pb.h"
#include "BoardImpl/BoardServiceImplFwd.hpp"


namespace board_module {

namespace contracts = online_desk::board;
    
class SessionReactor final
    : public grpc::ServerBidiReactor<online_desk::board::BoardUpdate, online_desk::board::BoardUpdate> {

    grpc::CallbackServerContext *context_;
    SessionManager& manager_;
    SessionInstance* session_instance_;
    std::atomic<bool> is_alive;

    std::atomic<bool> is_writing_;
    std::mutex write_mutex_;

    std::list<contracts::BoardUpdate> write_queue_;
    contracts::BoardUpdate request_;
    contracts::BoardUpdate response_;

    void ProcessQueue();
    void Broadcast(const contracts::BoardUpdate &request);
    void Shutdown();

public:
    explicit SessionReactor(grpc::CallbackServerContext *context, SessionManager& manager);

    void ProcessMessage(contracts::BoardUpdate msg);
    void OnWriteDone(bool ok) override;

    void OnReadDone(bool ok) override;

    void OnDone() override;
    void OnCancel() override;
};

}
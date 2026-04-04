#pragma once

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <thread>
#include "BoardImpl/BoardServiceImplFwd.hpp"
#include "BoardWidgets/BoardWidgets.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"

namespace board_module {

namespace contracts = online_desk::board;

class SessionInstance {

    friend class SessionReactor;

    SessionManager &manager_;
    uint64_t board_id_;
    std::mutex board_edit_mutex_;
    std::unordered_set<SessionReactor*> session_members_;
    std::unordered_map<uint64_t, widgets::BaseWidget> widgets_storage_;

    void CloseMemberConnection(SessionReactor* member);
    
public:

    SessionInstance* JoinToSession(SessionReactor* reactor);
    explicit SessionInstance(SessionManager& manager, uint64_t board_id);
};

class SessionManager {
    std::mutex sessions_table_mutex_;
    std::unordered_map<uint64_t, SessionInstance *> sessions_table_;

public:
    void CloseSession(uint64_t board_id);
    SessionInstance* JoinToSession(SessionReactor *member, uint64_t board_id);
};

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

    void ProcessMessage(const contracts::BoardUpdate &msg);
    void OnWriteDone(bool ok) override;

    void OnReadDone(bool ok) override;

    void OnDone() override;
    void OnCancel() override;
};

}  // namespace board_module

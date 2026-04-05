#pragma once

#include "board.grpc.pb.h"
#include "board.pb.h"
#include "BoardImpl/BoardServiceImplFwd.hpp"
#include <mutex>
#include <unordered_set>
#include <unordered_map>

namespace board_module {

namespace contracts = online_desk::board;

class SessionInstance {

    friend class SessionReactor;

    SessionManager &manager_;
    uint64_t board_id_;
    std::mutex board_edit_mutex_;
    std::unordered_set<SessionReactor*> session_members_;
    std::unordered_set<uint64_t> widgets_storage_;

    void CloseMemberConnection(SessionReactor* member);
    
public:

    SessionInstance* JoinToSession(SessionReactor* reactor);
    explicit SessionInstance(SessionManager& manager, uint64_t board_id);
};

}
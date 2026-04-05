#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "BoardServiceImplFwd.hpp"
#include "AuthenticationImpl/AuthenticationImpl.hpp"
#include "SessionImpl/SessionManager.hpp"
#include "board.grpc.pb.h"
#include "board.pb.h"

namespace board_module {

namespace contracts = online_desk::board;

class BoardsDataBase {

    mutable std::mutex db_edit_mutex_;
    std::unordered_map<uint64_t, std::string> boards_;

public:
    void SetBoard(uint64_t board_id, std::string board_name);
    std::optional<std::string> GetBoard(uint64_t board_id) const;

};

class BoardServiceImpl final : public contracts::BoardService::WithCallbackMethod_SubscribeBoard<contracts::BoardService::Service> {
    std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl_;
    BoardsDataBase data_base_;
    std::mt19937_64 create_rand_64_;
    std::unordered_map<std::string, std::vector<uint64_t>> user_owned_boards_;
    SessionManager session_manager_;

public:
    explicit BoardServiceImpl(
        std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl
    );
    ~BoardServiceImpl() override;

    std::string GetBoardName(uint64_t board_id) const;

    grpc::Status FetchUserBoards(
        grpc::ServerContext *context,
        const contracts::FetchUserBoardsRequest *request,
        contracts::FetchUserBoardsResponse *response
    ) override;

    grpc::Status CreateBoard(
        grpc::ServerContext *context,
        const contracts::CreateBoardRequest *request,
        contracts::CreateBoardResponse *response
    ) override;

    grpc::Status DeleteBoard(
        grpc::ServerContext *context,
        const contracts::DeleteBoardRequest *request,
        contracts::DeleteBoardResponse *response
    ) override;

    grpc::Status CreateBoardSnapshot(
        grpc::ServerContext *context,
        const contracts::CreateSnapshotRequest *request,
        contracts::CreateSnapshotResponse *response
    ) override;

    grpc::ServerBidiReactor<contracts::BoardUpdate, contracts::BoardUpdate>
        *SubscribeBoard(grpc::CallbackServerContext *context) override;
};

}  // namespace board_module

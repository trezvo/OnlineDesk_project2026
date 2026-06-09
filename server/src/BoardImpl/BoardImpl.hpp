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
#include "Database/BoardRepository.hpp"
#include "Database/UserRepository.hpp"
#include "Database/WidgetRepository.hpp"

namespace board_module {

namespace contracts = online_desk::board;

class BoardServiceImpl final : public contracts::BoardService::WithCallbackMethod_SubscribeBoard<contracts::BoardService::Service> {
    std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl_;
    std::shared_ptr<db::BoardRepository> board_table_;
    std::shared_ptr<db::UserRepository> user_table_;
    std::shared_ptr<db::WidgetRepository> widget_table;
    std::mt19937_64 create_rand_64_;
    SessionManager session_manager_;

public:
    explicit BoardServiceImpl(
        std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl,
        std::shared_ptr<ODBConnectionManager> conn_pool
    ) : auth_impl_(auth_impl), 
    user_table_(std::make_shared<db::UserRepository>(conn_pool)), 
    board_table_(std::make_shared<db::BoardRepository>(conn_pool)),
    widget_table(std::make_shared<db::WidgetRepository>(conn_pool)),
    session_manager_(widget_table, board_table_) {}
    
    ~BoardServiceImpl() override;

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

    grpc::Status RenameBoard(
        grpc::ServerContext *context,
        const contracts::RenameBoardRequest *request,
        contracts::RenameBoardResponse *response
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
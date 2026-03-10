#pragma once


#include <grpcpp/grpcpp.h>
#include "board.grpc.pb.h"
#include "board.pb.h"

#include "AuthenticationImpl.hpp"

#include <unordered_map>
#include <string>
#include <optional>
#include <vector>
#include <memory>


namespace board_module {

namespace contracts = online_desk::board;


class BoardsDataBase {


public:

    void SetBoard(uint64_t borad_id, std::string board_name);
    std::optional<std::string> GetBoard(uint64_t board_id) const;

private:
    std::vector<std::string> boards_;

};


class BoardServiceImpl final : public contracts::BoardService::Service {

    std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl_;
    BoardsDataBase data_base_;
    std::unordered_map<std::string, std::vector<uint64_t>> user_owned_boards_;

public:

    explicit BoardServiceImpl(std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl);
    ~BoardServiceImpl() override;

    std::string GetBoardName(uint64_t board_id) const;

    grpc::Status FetchUserBoards(
        grpc::ServerContext* context,
        const contracts::FetchUserBoardsRequest* request,
        contracts::FetchUserBoardsResponse* response
    ) override;

    grpc::Status CreateBoard (
        grpc::ServerContext* context,
        const contracts::CreateBoardRequest* request,
        contracts::CreateBoardResponse* response
    ) override;

    grpc::Status DeleteBoard (
        grpc::ServerContext* context,
        const contracts::DeleteBoardRequest* request,
        contracts::DeleteBoardResponse* response
    ) override;
};

}
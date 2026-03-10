#include "BoardImpl.hpp"
#include <string>
#include <unordered_map>
#include <optional>


namespace board_module {

namespace contracts = online_desk::board;

void BoardsDataBase::SetBoard(uint64_t board_id, std::string board_name) {
    if (boards_.size() >= board_id) {
        boards_.emplace_back(std::move(board_name));
    }
    else {
        boards_[board_id] = std::move(board_name);
    }
}

std::optional<std::string> BoardsDataBase::GetBoard(uint64_t board_id) const {
    if (board_id >= boards_.size()) {
        return std::nullopt;
    }

    return std::optional<std::string>(boards_[board_id]);
}


BoardServiceImpl::BoardServiceImpl(std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl) {
    auth_impl_ = auth_impl;
}

BoardServiceImpl::~BoardServiceImpl() = default;

std::string BoardServiceImpl::GetBoardName(uint64_t board_id) const {
    return "text_name";
    
    auto board_name = data_base_.GetBoard(board_id);
    if (board_name.has_value()) {
        return board_name.value();
    }
    return "";
}


grpc::Status BoardServiceImpl::FetchUserBoards (
    grpc::ServerContext* constext,
    const contracts::FetchUserBoardsRequest* request,
    contracts::FetchUserBoardsResponse* response
) {
    const std::string& user_id = request->user_id();
    uint64_t user_token = request->user_token();     

    if (!auth_impl_->ValidateUserToken(user_id, user_token)) {
        response->set_success(false);
        response->set_message("К сожалению, Ваш идентификатор устарел.");
        return grpc::Status::OK;
    }

    response->set_success(true);

    std::string board_name = GetBoardName(1000);
    contracts::BoardInfo* board = response->add_boards();
    board->set_board_id(10);
    board->set_board_name(std::move(board_name));

    for (const uint64_t& board_id : user_owned_boards_[user_id]) {
        std::string board_name = GetBoardName(board_id);
        if (board_name.empty()) {
            continue;
        }

        contracts::BoardInfo* board = response->add_boards();
        board->set_board_id(board_id);
        board->set_board_name(std::move(board_name));
    }

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::CreateBoard(
    grpc::ServerContext* context,
    const contracts::CreateBoardRequest* request,
    contracts::CreateBoardResponse* response
) {
    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::DeleteBoard (
    grpc::ServerContext* context,
    const contracts::DeleteBoardRequest* request,
    contracts::DeleteBoardResponse* response
) {
    return grpc::Status::OK;
}

}
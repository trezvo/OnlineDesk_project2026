#include "BoardImpl.hpp"
#include "SessionImpl/SessionReactor.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <chrono>
#include <iostream>

namespace board_module {

namespace contracts = online_desk::board;

void BoardsDataBase::SetBoard(uint64_t board_id, std::string board_name) {
    boards_[board_id] = std::move(board_name);
}

std::optional<std::string> BoardsDataBase::GetBoard(uint64_t board_id) const {
    std::lock_guard<std::mutex> lock(db_edit_mutex_);
    
    if (!boards_.contains(board_id)) {
        return std::nullopt;
    }

    return std::optional<std::string>(boards_.at(board_id));
}

BoardServiceImpl::BoardServiceImpl(
    std::shared_ptr<auth_module::AuthenticationServiceImpl> auth_impl
) {
    create_rand_64_ = std::mt19937_64(std::chrono::system_clock::now().time_since_epoch().count());
    auth_impl_ = auth_impl;
}

BoardServiceImpl::~BoardServiceImpl() = default;

std::string BoardServiceImpl::GetBoardName(uint64_t board_id) const {
    auto board_name = data_base_.GetBoard(board_id);
    if (!board_name.has_value()) {
        return "";
    }
    return board_name.value();
}

grpc::Status BoardServiceImpl::FetchUserBoards(
    grpc::ServerContext *constext,
    const contracts::FetchUserBoardsRequest *request,
    contracts::FetchUserBoardsResponse *response
) {
    const std::string &user_id = request->user_id();
    uint64_t user_token = request->user_token();

    if (!auth_impl_->ValidateUserToken(user_id, user_token)) {
        response->set_success(false);
        response->set_message("К сожалению, Ваш идентификатор устарел.");
        return grpc::Status::OK;
    }

    // std::cout << "fetch request from " << user_id << ", amout of its boards: " << user_owned_boards_[user_id].size() << std::endl;

    response->set_success(true);

    for (const uint64_t &board_id : user_owned_boards_[user_id]) {
        std::string board_name = GetBoardName(board_id);
        if (board_name.empty()) {
            std::cout << board_id << " - skipped" << std::endl;
            continue;
        }

        contracts::BoardInfo *board = response->add_boards();
        board->set_board_id(board_id);
        board->set_board_name(std::move(board_name));
    }

    // std::cout << "sending of list with size: " << response->boards_size() << std::endl;

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::CreateBoard(
    grpc::ServerContext *context,
    const contracts::CreateBoardRequest *request,
    contracts::CreateBoardResponse *response
) {
    std::string user_id = request->user_id();
    uint64_t user_token = request->user_token();
    std::string board_name = request->board_name();

    if (!auth_impl_->ValidateUserToken(user_id, user_token)) {
        response->set_success(false);
        response->set_message("К сожалению, Ваш идентификатор устарел.");
        return grpc::Status::OK;
    }

    if (board_name.empty()) {
        response->set_success(false);
        response->set_message("Название доски не может быть пустым");
        return grpc::Status::OK;
    }

    uint64_t new_board_id = create_rand_64_() % 10'000; // mod 1e5 so far 
    data_base_.SetBoard(new_board_id, board_name);
    user_owned_boards_[user_id].push_back(new_board_id);

    response->set_success(true);
    response->set_message("Доска создана");
    response->set_board_id(new_board_id);

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::DeleteBoard(
    grpc::ServerContext *context,
    const contracts::DeleteBoardRequest *request,
    contracts::DeleteBoardResponse *response
) {
    return grpc::Status::OK;
}

grpc::ServerBidiReactor<contracts::BoardUpdate, contracts::BoardUpdate>
    *BoardServiceImpl::SubscribeBoard(grpc::CallbackServerContext *context
    ) {
        return new SessionReactor(context, session_manager_);
}

}  // namespace board_module

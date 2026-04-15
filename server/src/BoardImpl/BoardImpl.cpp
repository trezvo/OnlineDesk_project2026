#include "BoardImpl.hpp"
#include "SessionImpl/SessionReactor.hpp"
#include <algorithm>
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

bool BoardsDataBase::RenameBoard(uint64_t board_id, std::string new_board_name) {
    std::lock_guard<std::mutex> lock(db_edit_mutex_);

    auto it = boards_.find(board_id);
    if (it == boards_.end()) {
        return false;
    }

    it->second = std::move(new_board_name);
    return true;
}

bool BoardsDataBase::DeleteBoard(uint64_t board_id) {
    std::lock_guard<std::mutex> lock(db_edit_mutex_);
    
    auto it = boards_.find(board_id);
    if (it == boards_.end()) {
        return false;
    }
    
    boards_.erase(it);
    return true;
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
    if (!auth_impl_->ValidateUserToken(request->user_id(), request->user_token())) {
        response->set_success(false);
        response->set_message("К сожалению, Ваш идентификатор устарел.");
        return grpc::Status::OK;
    }

    uint64_t board_id = request->board_id();
    const std::string& user_id = request->user_id();

    auto board_name = data_base_.GetBoard(board_id);
    if (!board_name.has_value()) {
        response->set_success(false);
        response->set_message("Доска не найдена");
        return grpc::Status::OK;
    }

    auto& user_boards = user_owned_boards_[user_id];
    auto it = std::find(user_boards.begin(), user_boards.end(), board_id);
    if (it == user_boards.end()) {
        response->set_success(false);
        response->set_message("У вас нет прав на удаление этой доски");
        return grpc::Status::OK;
    }

    contracts::BoardUpdate notification;
    notification.set_action_type(contracts::BOARD_DELETED);
    notification.set_widget_id(0);
    session_manager_.BroadcastToSession(board_id, notification);

    user_boards.erase(it);
    session_manager_.DeleteBoardWidgets(board_id);
    data_base_.DeleteBoard(board_id);
    session_manager_.CloseSession(board_id);

    response->set_success(true);
    response->set_message("Доска успешно удалена");

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::RenameBoard(
    grpc::ServerContext *context,
    const contracts::RenameBoardRequest *request,
    contracts::RenameBoardResponse *response
) {
    const std::string &user_id = request->user_id();
    uint64_t user_token = request->user_token();
    uint64_t board_id = request->board_id();
    std::string new_board_name = request->new_board_name();

    if (!auth_impl_->ValidateUserToken(user_id, user_token)) {
        response->set_success(false);
        response->set_message("К сожалению, ваш идентификатор устарел.");
        return grpc::Status::OK;
    }

    if (new_board_name.empty()) {
        response->set_success(false);
        response->set_message("Название доски не может быть пустым.");
        return grpc::Status::OK;
    }

    const auto &owned = user_owned_boards_[user_id];
    if (std::find(owned.begin(), owned.end(), board_id) == owned.end()) {
        response->set_success(false);
        response->set_message("У вас нет прав на переименование.");
        return grpc::Status::OK;
    }

    if (!data_base_.RenameBoard(board_id, std::move(new_board_name))) {
        response->set_success(false);
        response->set_message("Доска не найдена");
        return grpc::Status::OK;
    }

    response->set_success(true);
    response->set_message("Доска переименована");
    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::CreateBoardSnapshot(
    grpc::ServerContext *context,
    const contracts::CreateSnapshotRequest *request,
    contracts::CreateSnapshotResponse *response
) {
    uint64_t board_id = request->board_id();
    std::string user_id = request->user_id();

    uint64_t new_board_id = session_manager_.MakeBoardSnapshot(board_id);
    response->set_success(true);

    data_base_.SetBoard(new_board_id, "shapshot of " + std::to_string(board_id));
    user_owned_boards_[user_id].push_back(new_board_id);

    std::cout << "created snapshot of " << board_id << std::endl;

    return grpc::Status::OK;
}

grpc::ServerBidiReactor<contracts::BoardUpdate, contracts::BoardUpdate>
    *BoardServiceImpl::SubscribeBoard(grpc::CallbackServerContext *context
    ) {
    std::cout << "emit for subscribe" << std::endl;
    return new SessionReactor(context, session_manager_);
}

}  // namespace board_module
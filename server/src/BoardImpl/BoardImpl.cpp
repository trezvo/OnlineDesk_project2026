#include "BoardImpl.hpp"
#include "SessionImpl/SessionReactor.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <thread>
#include <variant>
#include "Models.hxx"

namespace board_module {

namespace contracts = online_desk::board;

BoardServiceImpl::~BoardServiceImpl() = default;

grpc::Status BoardServiceImpl::FetchUserBoards(
    grpc::ServerContext *context,
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

    response->set_success(true);

    for (db::Board& board_data : user_table_->FindOwnedBoards(user_id)) {
        contracts::BoardInfo *board = response->add_boards();
        board->set_board_id(board_data.id());
        board->set_board_name(std::move(board_data.name()));
    }

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

    db::Board board{ board_name };  
    
    switch (board_table_->create(board, user_id)) {
        using code = db::BoardRepository::CODE_ID;
        case (code::OK) : {    
            response->set_success(true);
            response->set_message("Доска создана");
            response->set_board_id(board.id());
        } break;
        case (code::BOARD_ALREADY_EXISTS) : {
            response->set_success(false);
            response->set_message("Вы уже владеете доской с таким именем");
        } break;
        case (code::USER_NOT_FOUND) : {
            response->set_success(false);
            response->set_message("Не получилось создать доску, перезагрузите приложение");
        } break;
        case (code::INTERNAL_ERROR) : {
            response->set_success(false);
            response->set_message("К сожалению, сервис не доступен");
        }
    }

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

    db::BoardRepository::findByIdResult board_query = 
                board_table_->findById(board_id);
    
    if (std::holds_alternative<db::BoardRepository::CODE_ID>(board_query)) {
        using code = db::BoardRepository::CODE_ID;
        switch (std::get<code>(board_query)) {
            case (code::BOARD_NOT_FOUND) : {
                response->set_success(false);
                response->set_message("Доска не найдена");
            } break;
        }

        return grpc::Status::OK;
    }

    db::Board& board_data = std::get<db::Board>(board_query); 

    if (board_data.owner()->uuid() != user_id) {
        response->set_success(false);
        response->set_message("У вас нет прав на удаление этой доски");
        return grpc::Status::OK;
    }

    contracts::BoardUpdate notification;
    notification.set_action_type(contracts::BOARD_DELETED);
    notification.set_widget_id(0);
    session_manager_.BroadcastToSession(board_id, notification);
    board_table_->remove(board_id);

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

    auto board_query = board_table_->findById(board_id);

    if (std::holds_alternative<db::BoardRepository::CODE_ID>(board_query)) {
        using code = db::BoardRepository::CODE_ID;
        switch (std::get<code>(board_query)) {
            case (code::BOARD_NOT_FOUND) : {
                response->set_success(false);
                response->set_message("Доска не найдена");
            } break;
        }

        return grpc::Status::OK;
    }

    db::Board& board_data = std::get<db::Board>(board_query); 

    if (board_data.owner()->uuid() != user_id) {
        response->set_success(false);
        response->set_message("У вас нет прав на удаление этой доски");
        return grpc::Status::OK;
    }

    board_data.name() = new_board_name;

    switch (board_table_->update(board_data)) {
        using code = db::BoardRepository::CODE_ID;

        case (code::OK) : {
            response->set_success(true);
            response->set_message("Доска переименована");
        } break;
        case (code::BOARD_NOT_FOUND) : {
            response->set_success(false);
            response->set_message("Доска не найдена");
        } break;
        case (code::INTERNAL_ERROR) : {
            response->set_success(false);
            response->set_message("Проблемы на сервере. Попробуйте позже");
        } break;
    }

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::CreateBoardSnapshot(
    grpc::ServerContext *context,
    const contracts::CreateSnapshotRequest *request,
    contracts::CreateSnapshotResponse *response
) {
    uint64_t board_id = request->board_id();
    std::string user_id = request->user_id();

    auto board_query = board_table_->findById(board_id);

    if (std::holds_alternative<db::BoardRepository::CODE_ID>(board_query)) {
        using code = db::BoardRepository::CODE_ID;
        switch (std::get<code>(board_query)) {
            case (code::BOARD_NOT_FOUND) : {
                response->set_success(false);
            } break;
        }

        return grpc::Status::OK;
    }

    db::Board& board = std::get<db::Board>(board_query);
    
    response->set_success(true);

    board.id() = 0;
    board.name() = "shapshot of " + std::to_string(board_id);

    board_table_->create(board, request->user_id());
    session_manager_.MakeBoardSnapshot(board_id, board.id());

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
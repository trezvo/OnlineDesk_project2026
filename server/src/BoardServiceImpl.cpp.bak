#include "BoardServiceImpl.hpp"
#include <iostream>

using namespace online_desk::board;

BoardServiceImpl::BoardServiceImpl() {}

BoardServiceImpl::~BoardServiceImpl() = default;

uint64_t BoardServiceImpl::generateBoardId() {
    return board_counter_++;
}

grpc::Status BoardServiceImpl::FetchUserBoards(
    grpc::ServerContext* context,
    const FetchUserBoardsRequest* request,
    FetchUserBoardsResponse* response) {

    uint64_t token = request->user_token();

    std::lock_guard<std::mutex> lock(boards_mutex_);

    auto it = user_boards_.find(token);
    if (it == user_boards_.end()) {
        response->set_success(true);
        response->set_message("No boards");
        return grpc::Status::OK;
    }

    for (auto board_id : it->second) {
        auto board_it = boards_.find(board_id);
        if (board_it == boards_.end())
            continue;

        auto* info = response->add_boards();
        info->set_board_id(board_it->second->board_id);
        info->set_board_name(board_it->second->board_name);
    }

    response->set_success(true);
    response->set_message("Boards fetched");

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::CreateBoard(
    grpc::ServerContext* context,
    const CreateBoardRequest* request,
    CreateBoardResponse* response) {

    uint64_t token = request->user_token();
    std::string name = request->board_name();

    if (name.empty()) {
        response->set_success(false);
        response->set_message("Board name is empty, try again");
        return grpc::Status::OK;
    }

    std::lock_guard<std::mutex> lock(boards_mutex_);

    uint64_t id = generateBoardId();

    auto board = std::make_shared<BoardState>();
    board->board_id = id;
    board->board_name = name;

    boards_[id] = board;
    user_boards_[token].push_back(id);

    response->set_success(true);
    response->set_message("Board created");
    response->set_board_id(id);

    std::cout << "Board created id = " << id << std::endl;

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::DeleteBoard(
    grpc::ServerContext* context,
    const DeleteBoardRequest* request,
    DeleteBoardResponse* response) {

    uint64_t token = request->user_token();
    uint64_t board_id = request->board_id();

    std::lock_guard<std::mutex> lock(boards_mutex_);

    auto it = boards_.find(board_id);

    if (it == boards_.end()) {
        response->set_success(false);
        response->set_message("Board not found");
        return grpc::Status::OK;
    }

    boards_.erase(it);

    auto user_it = user_boards_.find(token);
    if (user_it != user_boards_.end()) {
        auto& vec = user_it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), board_id), vec.end());
    }

    response->set_success(true);
    response->set_message("Board deleted");

    std::cout << "Board deleted id = " << board_id << std::endl;

    return grpc::Status::OK;
}

grpc::Status BoardServiceImpl::SubscribeBoard(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<SubscribeStreamChunck, SubscribeStreamChunck>* stream) {

    SubscribeStreamChunck request;

    while (stream->Read(&request)) {

        std::lock_guard<std::mutex> lock(boards_mutex_);

        for (auto& [id, board] : boards_) {

            std::lock_guard<std::mutex> board_lock(board->board_mutex);

            if (request.edit_type() == WidgetEditType::CREATE ||
                request.edit_type() == WidgetEditType::UPDATE) {

                WidgetState widget;
                widget.widget_id = request.widget_id();
                widget.info = request.update_data();

                board->widgets[widget.widget_id] = widget;
            }

            if (request.edit_type() == WidgetEditType::DELETE) {
                board->widgets.erase(request.widget_id());
            }
        }

        stream->Write(request);
    }

    std::cout << "Client disconnected" << std::endl;

    return grpc::Status::OK;
}

#pragma once

#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

#include "board.grpc.pb.h"

class BoardServiceImpl final : public online_desk::board::BoardService::Service {
public:
    BoardServiceImpl();
    ~BoardServiceImpl() override;

    grpc::Status FetchUserBoards(
        grpc::ServerContext* context,
        const online_desk::board::FetchUserBoardsRequest* request,
        online_desk::board::FetchUserBoardsResponse* response) override;

    grpc::Status CreateBoard(
        grpc::ServerContext* context,
        const online_desk::board::CreateBoardRequest* request,
        online_desk::board::CreateBoardResponse* response) override;

    grpc::Status DeleteBoard(
        grpc::ServerContext* context,
        const online_desk::board::DeleteBoardRequest* request,
        online_desk::board::DeleteBoardResponse* response) override;

    grpc::Status SubscribeBoard(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<
            online_desk::board::SubscribeStreamChunck,
            online_desk::board::SubscribeStreamChunck>* stream) override;

private:

    struct WidgetState {
        std::string widget_id;
        online_desk::board::WidgetInfo info;
    };

    struct BoardState {
        uint64_t board_id;
        std::string board_name;

        std::unordered_map<std::string, WidgetState> widgets;

        std::mutex board_mutex;
    };

    uint64_t generateBoardId();

    std::mutex boards_mutex_;

    std::unordered_map<uint64_t, std::shared_ptr<BoardState>> boards_;

    std::unordered_map<uint64_t, std::vector<uint64_t>> user_boards_;

    std::atomic<uint64_t> board_counter_{1};
};

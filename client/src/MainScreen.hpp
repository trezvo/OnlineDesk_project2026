#pragma once
#include "GrpcBoardClient.hpp"
#include <QMainWindow>
#include <memory> 
#include <vector>

class MainScreen : public QMainWindow {

    struct Board {
        uint32_t board_id;
    };

    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    std::vector<Board> boards_list_;

    void FetchBoardList() {} //TODO
    void DisplayBoards() {} //TODO
    void OpenBoard() {} //TODO

public:

    explicit MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QWidget* parent = nullptr);

};

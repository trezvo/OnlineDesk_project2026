#pragma once

#include "GrpcBoardClient.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <memory>

class BoardScreen : public QMainWindow {
    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;

public:

    explicit BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QWidget* parent = nullptr);
    ~BoardScreen() = default;
};

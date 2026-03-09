#pragma once

#include "GrpcBoardClient"
#include "BoardsListWidget.hpp"
#include <QMainWindow>
#include <memory>

class BoardScreen : public QMainWindow {
    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;

public:

    explicit BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent = nullptr);
    ~BoardScreen() {}
}
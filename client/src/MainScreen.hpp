#pragma once
#include "GrpcBoardClient.hpp"
#include "AppController.hpp"
#include "BoardsListWidget.hpp"
#include <QMainWindow>
#include <QHBoxLayout>
#include <memory> 
#include <vector>

class MainScreen : public QMainWindow {

    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    std::shared_ptr<AppController> app_;
    std::unique_ptr<BoardsListWidget> boards_list_;
    std::unique_ptr<QHBoxLayout> layout_;

public slots:
    void onBoardCreateClicked();
    void onBoardJoinClicked();

public:

    explicit MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, std::shared_ptr<AppController> app, QWidget* parent = nullptr);

};

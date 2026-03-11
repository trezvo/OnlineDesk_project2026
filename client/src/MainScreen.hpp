#pragma once
#include "GrpcBoardClient.hpp"
#include "AppControllerFwd.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QHBoxLayout>
#include <memory> 
#include <vector>

class MainScreen : public QMainWindow {

    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    std::shared_ptr<AppController> app_;
    BoardsButtonList* boards_list_;
    QHBoxLayout* layout_;

// public slots:
//     void onBoardCreateClicked();
//     void onBoardJoinClicked();
//TODO

public:

    explicit MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, std::shared_ptr<AppController> app, QWidget* parent = nullptr);


private slots:

    void onCreateBoardClicked();

};

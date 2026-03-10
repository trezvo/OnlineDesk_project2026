#include "GrpcBoardClient.hpp"
#include "AppController.hpp"
#include "MainScreen.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QHBoxLayout>
#include <memory>


MainScreen::MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, std::shared_ptr<AppController> app, QWidget* parent) 
    : QMainWindow(parent) 
    , grpc_client_(grpc_client)
    , app_(app) {
    SetupUI();
}

void MainScreen::SetupUI() {
    
    setWindowTitle("Главное меню");

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    layout_ = new QHBoxLayout(centralWidget);
    layout_->setContentsMargins(5, 5, 5, 5);
    layout_->setSpacing(10);

    boards_list_ = new BoardsButtonList(grpc_client_, app_, this);
    layout_->addWidget(boards_list_);

    resize(800, 600);
}
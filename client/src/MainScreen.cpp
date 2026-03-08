#include "GrpcBoardClient.hpp"
#include "MainScreen.hpp"
#include <memory>


MainScreen::MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QWidget* parent) 
    : QMainWindow(parent) 
    , grpc_client_(grpc_client) {
    SetupUI();
    FetchBoardList();
    DisplayBoards();
}

void MainScreen::SetupUI() {
    //deepseeked ex

    setWindowTitle("Главное окно");
    resize(800, 600);
        
    // Устанавливаем центральный виджет (обязательно для QMainWindow!)
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
        
    // Дополнительно: можно установить цвет фона
    centralWidget->setStyleSheet("background-color: #ed9898;");
}
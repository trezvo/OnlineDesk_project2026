#include "GrpcBoardClient.hpp"
#include "BoardScreen.hpp"
#include <memory>

BoardScreen::BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QWidget* parent)
    : QMainWindow(parent)
    , grpc_client_(grpc_client) {
    SetupUI();
}

void BoardScreen::SetupUI() {
     //deepseeked ex

    setWindowTitle("Окно сессии");
    resize(800, 600);
    
    // Устанавливаем центральный виджет (обязательно для QMainWindow!)
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
        
    // Дополнительно: можно установить цвет фона
    centralWidget->setStyleSheet("background-color: #ed9898;");
}

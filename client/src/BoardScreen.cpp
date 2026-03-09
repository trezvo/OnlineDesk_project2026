#include "GrpcBoardClient"
#include "BoardScreen.hpp"
#include <memory>

BoardScreen::BoardScreen(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent)
    : QObject(parent)
    , grpc_client_(grpc_client) {
    SetupUI();
}

BoardScreen::SetupUi() {
     //deepseeked ex

    setWindowTitle("Главное окно");
    resize(800, 600);
    
    // Устанавливаем центральный виджет (обязательно для QMainWindow!)
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
        
    // Дополнительно: можно установить цвет фона
    centralWidget->setStyleSheet("background-color: #ed9898;");
}

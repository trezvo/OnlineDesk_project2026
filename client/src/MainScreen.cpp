#include "GrpcBoardClient.hpp"
#include "AppController.hpp"
#include "MainScreen.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox> 
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

    QPushButton* create_board_ = new QPushButton("Создать доску", this);
    create_board_->setFixedHeight(40);
    layout_->addWidget(create_board_);

    connect(create_board_, &QPushButton::clicked, this,  &MainScreen::onCreateBoardClicked);

    resize(800, 600);
}

void MainScreen::onCreateBoardClicked(){
    bool ok;
    QString board_name = QInputDialog::getText(this, "Создание доски", "Введите название доски", QLineEdit::Normal, "", &ok);

    if (!ok || board_name.isEmpty()){
        return;
    }
    auto result = grpc_client_->createBoard(board_name.toStdString());
    
    if (result.success) {
        QMessageBox::information(this, "Успех", "Доска создана!");
        boards_list_->UpdateUI();
    }
    else {
        QMessageBox::warning(this, "Ошибка", 
            QString::fromStdString(result.message));
    }
}
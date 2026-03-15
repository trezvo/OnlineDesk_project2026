#include "GrpcBoardClient.hpp"
#include "AppController.hpp"
#include "MainScreen.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QMessageBox> 
#include <memory>

MainScreen::MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, AppController& app, QWidget* parent) 
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

    QVBoxLayout* board_party = new QVBoxLayout(centralWidget);
    
    QLabel* party_label = new QLabel("Присоединиться к доске:", centralWidget);
    board_party->addWidget(party_label);    

    lobby_id_line_ = new QLineEdit(centralWidget);
    lobby_id_line_->setPlaceholderText("id доски");
    lobby_id_line_->setMinimumHeight(20);
    board_party->addWidget(lobby_id_line_);

    lobby_join_ = new QPushButton(centralWidget);
    lobby_join_ ->setMinimumHeight(40);
    board_party->addWidget(lobby_join_);

    layout_->addLayout(board_party);

    connect(create_board_, &QPushButton::clicked, this,  &MainScreen::onCreateBoardClicked);
    connect(this, &MainScreen::onMainScreenFinished, &app_, &AppController::onMainScreenFinished);
    connect(lobby_join_, &QPushButton::clicked, this, &MainScreen::onJoinPartyClicked);

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

void MainScreen::onJoinPartyClicked() {

    QString line_input = lobby_id_line_->text();

    if (line_input.isEmpty()) {
        return;
    }

    uint64_t board_id = std::stoi(line_input.toStdString());
    emit onMainScreenFinished(board_id);

}
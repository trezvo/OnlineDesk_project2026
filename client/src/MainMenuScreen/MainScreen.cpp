#include "AppController/AppController.hpp"
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

    lobby_id_line_ = new QLineEdit(this);
    lobby_id_line_->setPlaceholderText("id доски");
    lobby_id_line_->setMinimumHeight(20);
    layout_->addWidget(lobby_id_line_);

    lobby_join_ = new QPushButton("Присоединиться к сессии: ", this);
    lobby_join_ ->setMinimumHeight(40);
    layout_->addWidget(lobby_join_);

    rename_id_line_ = new QLineEdit(this);
    rename_id_line_->setPlaceholderText("id доски");
    rename_id_line_->setMinimumHeight(20);
    layout_->addWidget(rename_id_line_);

    rename_board_ = new QPushButton("Переименовать доску", this);
    rename_board_->setMinimumHeight(40);
    layout_->addWidget(rename_board_);

    connect(create_board_, &QPushButton::clicked, this,  &MainScreen::onCreateBoardClicked);
    connect(this, &MainScreen::onMainScreenFinished, &app_, &AppController::onMainScreenFinished);
    connect(lobby_join_, &QPushButton::clicked, this, &MainScreen::onJoinPartyClicked);
    connect(rename_board_, &QPushButton::clicked, this, &MainScreen::onRenameBoardClicked);

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

    uint64_t board_id = std::stoull(line_input.toStdString());
    emit onMainScreenFinished(board_id);

}

void MainScreen::onRenameBoardClicked() {
    QString line_input = rename_id_line_->text();

    if (line_input.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите id доски");
        return;
    }

    uint64_t board_id = 0;
    try {
        board_id = std::stoull(line_input.toStdString());
    } catch (const std::exception&) {
        QMessageBox::warning(this, "Ошибка", "Некорректный id доски");
        return;
    }

    bool ok;
    QString new_name = QInputDialog::getText(
        this, "Переименование доски", "Введите новое название доски",
        QLineEdit::Normal, "", &ok);

    if (!ok || new_name.isEmpty()) {
        return;
    }

    auto result = grpc_client_->renameBoard(board_id, new_name.toStdString());

    if (result.success) {
        QMessageBox::information(this, "Успех", "Доска переименована!");
        rename_id_line_->clear();
        boards_list_->UpdateUI();
    } else {
        QMessageBox::warning(this, "Ошибка",
                             QString::fromStdString(result.message));
    }
}
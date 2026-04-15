#include "AppController/AppController.hpp"
#include "MainScreen.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QTimer>
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

    connect(boards_list_, &BoardsButtonList::deleteBoardRequested, 
            this, &MainScreen::onDeleteBoardFromList);

    QPushButton* create_board_ = new QPushButton("Создать доску", this);
    create_board_->setFixedHeight(40);
    layout_->addWidget(create_board_);

    QVBoxLayout* menu_layout_ = new QVBoxLayout();
    layout_->addLayout(menu_layout_);

    QHBoxLayout* join_layout_ = new QHBoxLayout();
    menu_layout_->addLayout(join_layout_);
    
    lobby_id_line_ = new QLineEdit(this);
    lobby_id_line_->setPlaceholderText("id доски");
    lobby_id_line_->setMinimumHeight(20);
    join_layout_->addWidget(lobby_id_line_);

    lobby_join_ = new QPushButton("Присоединиться к сессии: ", this);
    lobby_join_ ->setMinimumHeight(40);
    join_layout_->addWidget(lobby_join_);

    QHBoxLayout* rename_layout_ = new QHBoxLayout();
    menu_layout_->addLayout(rename_layout_);

    rename_id_line_ = new QLineEdit(this);
    rename_id_line_->setPlaceholderText("id доски");
    rename_id_line_->setMinimumHeight(20);
    rename_layout_->addWidget(rename_id_line_);

    rename_board_ = new QPushButton("Переименовать доску", this);
    rename_board_->setMinimumHeight(40);
    rename_layout_->addWidget(rename_board_);

    QHBoxLayout* delete_layout = new QHBoxLayout();
    menu_layout_->addLayout(delete_layout);

    delete_id_line_ = new QLineEdit(this);
    delete_id_line_->setPlaceholderText("id доски для удаления");
    delete_id_line_->setMinimumHeight(20);
    delete_layout->addWidget(delete_id_line_);

    delete_board_ = new QPushButton("Удалить доску", this);
    delete_board_->setMinimumHeight(40);
    delete_board_->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF3B30;"
        "  color: white;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #D70015;"
        "}"
    );
    delete_layout->addWidget(delete_board_);

    connect(create_board_, &QPushButton::clicked, this,  &MainScreen::onCreateBoardClicked);
    connect(this, &MainScreen::onMainScreenFinished, &app_, &AppController::onMainScreenFinished);
    connect(lobby_join_, &QPushButton::clicked, this, &MainScreen::onJoinPartyClicked);
    connect(rename_board_, &QPushButton::clicked, this, &MainScreen::onRenameBoardClicked);
    connect(delete_board_, &QPushButton::clicked, this, &MainScreen::onDeleteBoardClicked);

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

void MainScreen::onDeleteBoardClicked() {
    QString line_input = delete_id_line_->text();

    if (line_input.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите id доски для удаления");
        return;
    }

    uint64_t board_id = 0;
    try {
        board_id = std::stoull(line_input.toStdString());
    } catch (const std::exception&) {
        QMessageBox::warning(this, "Ошибка", "Некорректный id доски");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Подтверждение удаления", 
        QString("Вы уверены, что хотите удалить доску %1?\nЭто действие необратимо!").arg(board_id),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    auto result = grpc_client_->deleteBoard(board_id);
    if (result.success) {
        QMessageBox::information(this, "Успех", "Доска успешно удалена!");
        delete_id_line_->clear();
        QTimer::singleShot(0, this, [this]() {
            boards_list_->UpdateUI();
        });
    } else {
        QMessageBox::warning(this, "Ошибка",
                             QString::fromStdString(result.message));
    }
}

void MainScreen::onDeleteBoardFromList(uint64_t board_id) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Подтверждение удаления", 
        QString("Вы уверены, что хотите удалить доску %1?\nЭто действие необратимо!").arg(board_id),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    auto result = grpc_client_->deleteBoard(board_id);

    if (result.success) {
        QMessageBox::information(this, "Успех", "Доска успешно удалена!");
        QTimer::singleShot(0, this, [this]() {
            boards_list_->UpdateUI();
        });
    } else {
        QMessageBox::warning(this, "Ошибка",
                             QString::fromStdString(result.message));
    }
}
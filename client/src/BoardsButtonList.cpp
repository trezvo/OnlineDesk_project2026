#include "GrpcBoardClient.hpp"
#include "AppController.hpp"
#include "BoardsButtonList.hpp"
#include <QString>
#include <QPushButton>
#include <QHBoxLayout>
#include <memory>
#include <string>
#include <utility>


BoardButton::BoardButton(const QString& board_name, uint64_t board_id, QWidget* parent)
    : QPushButton(board_name, parent)
    , board_id_(std::move(board_id)) {
}

void BoardButton::mouseDoubleClickEvent(QMouseEvent* event) {
    emit onDoubleClickedBoardButton(board_id_);
}

BoardsButtonList::BoardsButtonList(std::shared_ptr<GrpcBoardClient> grpc_client, std::shared_ptr<AppController> app, QWidget* parent)
    : QWidget(parent)
    , grpc_client_(grpc_client)
    , app_(app) {

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(2);
    layout_->setAlignment(Qt::AlignTop);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    UpdateUI();
}


void BoardsButtonList::clear() {
    
    for (BoardButton* BoardPointer : buttons_) {
        BoardPointer->setParent(nullptr);
    }

    qDeleteAll(buttons_);
    buttons_.clear();

    while (QLayoutItem* item = layout_->takeAt(0)) {
        delete item;
    }

    layout_->addStretch();
}


void BoardsButtonList::UpdateUI() {

    this->hide();
    this->clear();
    auto owned_boards = grpc_client_->fetchUserBoards();

    if (!owned_boards.first) {
        return;
    }

    for (const auto& [id, name] : owned_boards.second) {
        BoardButton* new_button = new BoardButton(QString::fromStdString(name), id, this);
        new_button->setFixedHeight(20);
        new_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(new_button, &BoardButton::onDoubleClickedBoardButton, 
                app_.get(), &AppController::onMainScreenFinished);
        
        layout_->addWidget(new_button);
        buttons_.append(new_button);
    }

    layout_->addStretch();
    this->show();
}
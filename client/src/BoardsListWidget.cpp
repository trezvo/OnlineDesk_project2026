#include "AppController.hpp"
#include "BoardsListWidget.hpp"
#include <memory>

BoardButton::BoardButton(const QString& board_name, uint32_t board_id, std::shared_ptr<AppController> app, QObject* parent)
    : QPushButton(board_name, parent)
    , board_id_(board_id)
    , app_(app) {
    connect(this, &BoardButton::doubleClickedBoardButton, app_.get(), &AppController::onMainScreenFinished);
}

void mouseDoubleClickEvent(QMouseEvent* event) {
    emit doubleClickedBoardButton(board_id_);
}


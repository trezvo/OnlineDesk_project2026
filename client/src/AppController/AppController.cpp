#include "AppController.hpp"
#include "Registration/RegisterDialog.hpp"
#include "MainMenuScreen/BoardsButtonList.hpp"

#include <iostream>
#include <memory>

AppController::AppController(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent)
    : QObject(parent)
    , grpc_client_(grpc_client) {
}

void AppController::showAuthDialog() {
    auth_dialog_ = new AuthDialog(grpc_client_);
    auth_dialog_->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(auth_dialog_, &AuthDialog::loginConfirmed, this, &AppController::onAuthDialogFinished);
    auth_dialog_->show();

    QObject::connect(auth_dialog_, &AuthDialog::registerRequested, [this]() {
        RegisterDialog* reg = new RegisterDialog(grpc_client_);
        reg->setAttribute(Qt::WA_DeleteOnClose);
        reg->exec();
    });

}

void AppController::onAuthDialogFinished() {

    if (grpc_client_->get_login_data().success) {
        showMainScreen();
    }
}

void AppController::showMainScreen() {
    main_screen_ = new MainScreen(grpc_client_, *this);
    main_screen_->setAttribute(Qt::WA_DeleteOnClose);
    main_screen_->show();
}

void AppController::onMainScreenFinished(uint64_t board_id) {
    main_screen_->close();
    showBoardScreen(board_id);
}

void AppController::showBoardScreen(uint64_t board_id) {
    board_screen_ = new BoardScreen(grpc_client_, board_id);
    board_screen_->setAttribute(Qt::WA_DeleteOnClose);
    board_screen_->show();
}

void AppController::run() {
    showAuthDialog();
}

#include "AppController.hpp"
#include "AuthDialog.hpp"
#include "RegisterDialog.hpp"
#include "GrpcBoardClient.hpp"

#include <iostream>
#include <memory>

AppController::AppController(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent)
    : QObject(parent)
    , grpc_client_(grpc_client) {
}

void AppController::showAuthDialog() {
    auth_dialog_.reset(new AuthDialog(grpc_client_));
    auth_dialog_->setAttribute(Qt::WA_DeleteOnClose);
    auth_dialog_->show();

    QObject::connect(auth_dialog_, &AuthDialog::registerRequested, [this]() {
        RegisterDialog* reg = new RegisterDialog(grpc_client_);
        reg->setAttribute(Qt::WA_DeleteOnClose);
        reg->exec();
    });

}

void AppController::onAuthDialogFineshed() {

    auth_dialog_->deleteLater();
    if (grpc_client_->get_login_data().success) {
        showMainScreen();
    }
}

void AppController::showMainScreen() {

    main_screen_.reset(new MainScreen(grpc_client_));
    MainScreen->show();
}

void AppController::onMainScreenFinished(uint32_t board_id) {

    main_screen_->deleteLater();

    // здесь нужно написать fetch доски с сервера, вероятно
    // именно здесь инициализируем поток сервер -> клиент

    showBoardScreen();
}

void AppController::run() {
    showAuthDialog();
}
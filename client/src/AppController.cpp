#include "AppController.hpp"
#include "AuthDialog.hpp"
#include "RegisterDialog.hpp"

#include <iostream>

void AppController::run() {

    AuthDialog* auth_dialog_ = new AuthDialog(grpc_client_);
    auth_dialog_->setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(auth_dialog_, &AuthDialog::registerRequested, [this]() {
        RegisterDialog* reg = new RegisterDialog(grpc_client_);
        reg->setAttribute(Qt::WA_DeleteOnClose);
        reg->show();
    });


    if (auth_dialog_->exec() == QDialog::Accepted && grpc_client_->get_login_data().success) {
        main_window_ = new MainScreen(grpc_client_);
        main_window_->show();
    }
    else {
        // это дикик костыль надо переделать AuthDialog: QDialog -> QMainWidget, 
        // иначе app.exec() не считает его открытым окном из-за модальности (auth_dialog_->exec())
        // и не завершает программу после "отмены"
        exit(0);
    }
}
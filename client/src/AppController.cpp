#include "AppController.hpp"
#include "AuthDialog.hpp"
#include "RegisterDialog.hpp"

#include <iostream>

void AppController::run() {

    auth_dialog_ = std::make_unique<AuthDialog>(grpc_client_);
    
    QObject::connect(auth_dialog_.get(), &AuthDialog::registerRequested, [this]() {
        RegisterDialog* reg = new RegisterDialog(grpc_client_);
        reg->setAttribute(Qt::WA_DeleteOnClose);
        reg->show();
    });

    if (auth_dialog_->exec() == QDialog::Accepted && grpc_client_->get_login_data().success) {
        main_window_ = std::make_unique<MainScreen>(grpc_client_);
        main_window_->show();
    }
}
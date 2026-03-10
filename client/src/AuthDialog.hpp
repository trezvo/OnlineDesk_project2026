#pragma once

#include "GrpcBoardClient.hpp"
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>


class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(std::shared_ptr<GrpcBoardClient> client, QWidget* parent = nullptr);

signals:
    void registerRequested();
    void loginConfirmed();

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    QLineEdit* username_edit_;
    QLineEdit* password_edit_;
    QPushButton* login_button_;
    QPushButton* register_button_; 
    QPushButton* cancel_button_;
    
    QLabel* error_label_;
};
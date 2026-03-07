#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>

class GrpcBoardClient;

class RegisterDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterDialog(std::shared_ptr<GrpcBoardClient> client, QWidget* parent = nullptr);

private slots:
    void onRegisterClicked();

private:
    void setupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    QLineEdit* username_edit_;
    QLineEdit* password_edit_;
    QLineEdit* password_confirm_edit_;
    QPushButton* register_button_;
    QPushButton* cancel_button_;
    QLabel* error_label_;
};
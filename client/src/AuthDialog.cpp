#include "AuthDialog.hpp"
#include "GrpcBoardClient.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>

AuthDialog::AuthDialog(std::shared_ptr<GrpcBoardClient> client, QWidget* parent)
    : QDialog(parent)
    , grpc_client_(client) {
    setupUI();
}

void AuthDialog::setupUI() {
    setWindowTitle("Online Desk");
    setModal(true);
    setFixedSize(400, 275);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);

    auto* title_label = new QLabel("Вход в систему", this);
    title_label->setAlignment(Qt::AlignCenter);
    QFont title_font = title_label->font();
    title_font.setPointSize(16);
    title_font.setBold(true);
    title_label->setFont(title_font);
    layout->addWidget(title_label);

    auto* form_layout = new QFormLayout();
    form_layout->setSpacing(10);

    username_edit_ = new QLineEdit(this);
    username_edit_->setPlaceholderText("Введите имя пользователя");
    username_edit_->setMinimumHeight(35);
    form_layout->addRow("Логин:", username_edit_);

    password_edit_ = new QLineEdit(this);
    password_edit_->setPlaceholderText("Введите пароль");
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setMinimumHeight(35);
    form_layout->addRow("Пароль:", password_edit_);

    layout->addLayout(form_layout);

    error_label_ = new QLabel(this);
    error_label_->setStyleSheet("QLabel { color: red; }");
    error_label_->setAlignment(Qt::AlignCenter);
    error_label_->setWordWrap(true);
    error_label_->hide();
    layout->addWidget(error_label_);

    layout->addStretch();

    auto* button_layout = new QHBoxLayout();
    
    cancel_button_ = new QPushButton("Отмена", this);
    cancel_button_->setMinimumHeight(40);
    button_layout->addWidget(cancel_button_);

    register_button_ = new QPushButton("Регистрация", this);
    register_button_->setMinimumHeight(40);
    button_layout->addWidget(register_button_);

    login_button_ = new QPushButton("Войти", this);
    login_button_->setMinimumHeight(40);
    login_button_->setStyleSheet(
        "QPushButton {"
        "  background-color: #007AFF;"
        "  color: white;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0051D5;"
        "}"
    );
    button_layout->addWidget(login_button_);

    layout->addLayout(button_layout);

    connect(register_button_, &QPushButton::clicked, this, &AuthDialog::onRegisterClicked);
    connect(login_button_, &QPushButton::clicked, this, &AuthDialog::onLoginClicked);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
    connect(username_edit_, &QLineEdit::returnPressed, this, &AuthDialog::onLoginClicked);
    connect(password_edit_, &QLineEdit::returnPressed, this, &AuthDialog::onLoginClicked);
}

void AuthDialog::onLoginClicked() {
    QString username = username_edit_->text().trimmed();
    QString password = password_edit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        error_label_->setText("Заполните все поля");
        error_label_->show();
        return;
    }

    error_label_->hide();
    login_button_->setEnabled(false);
    login_button_->setText("Вход...");

    grpc_client_->login(username.toStdString(), password.toStdString());

    const auto& login_result = grpc_client_->get_login_data();

    login_button_->setEnabled(true);
    login_button_->setText("Войти");

    if (login_result.success) {
        accept();
        emit loginConfirmed();
    } else {
        error_label_->setText(QString::fromStdString(login_result.message));
        error_label_->show();
    }
}

void AuthDialog::onRegisterClicked() {

    emit registerRequested();
}

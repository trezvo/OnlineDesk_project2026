#include "RegisterDialog.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>

RegisterDialog::RegisterDialog(std::shared_ptr<GrpcBoardClient> grpc_client, QWidget* parent)
    : QDialog(parent)
    , grpc_client_(grpc_client) {
    setupUI();
}

void RegisterDialog::setupUI() {
    setWindowTitle("Регистрация");
    setModal(true);
    setFixedSize(400, 275);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);

    auto* title_label = new QLabel("Создание аккаунта", this);
    title_label->setAlignment(Qt::AlignCenter);
    QFont title_font = title_label->font();
    title_font.setPointSize(16);
    title_font.setBold(true);
    title_label->setFont(title_font);
    layout->addWidget(title_label);

    auto* form_layout = new QFormLayout();
    form_layout->setSpacing(10);

    username_edit_ = new QLineEdit(this);
    username_edit_->setPlaceholderText("Выберите имя пользователя");
    username_edit_->setMinimumHeight(35);
    form_layout->addRow("Логин:", username_edit_);

    password_edit_ = new QLineEdit(this);
    password_edit_->setPlaceholderText("Придумайте пароль");
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setMinimumHeight(35);
    form_layout->addRow("Пароль:", password_edit_);

    password_confirm_edit_ = new QLineEdit(this);
    password_confirm_edit_->setPlaceholderText("Повторите пароль");
    password_confirm_edit_->setEchoMode(QLineEdit::Password);
    password_confirm_edit_->setMinimumHeight(35);
    form_layout->addRow("Подтверждение:", password_confirm_edit_);

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

    register_button_ = new QPushButton("Зарегистрироваться", this);
    register_button_->setMinimumHeight(40);
    register_button_->setStyleSheet(
        "QPushButton {"
        "  background-color: #34C759;"
        "  color: white;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #28A745;"
        "}"
    );
    button_layout->addWidget(register_button_);

    layout->addLayout(button_layout);

    connect(register_button_, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
    connect(password_confirm_edit_, &QLineEdit::returnPressed, this, &RegisterDialog::onRegisterClicked);
}

void RegisterDialog::onRegisterClicked() {
    QString username = username_edit_->text().trimmed();
    QString password = password_edit_->text();
    QString password_confirm = password_confirm_edit_->text();

    if (username.isEmpty() || password.isEmpty() || password_confirm.isEmpty()) {
        error_label_->setText("Заполните все поля");
        error_label_->show();
        return;
    }
    /*
    if (username.length() < 3) {
        error_label_->setText("Логин должен быть не менее 3 символов");
        error_label_->show();
        return;
    }

    if (password.length() < 4) {
        error_label_->setText("Пароль должен быть не менее 4 символов");
        error_label_->show();
        return;
    }
    */
    if (password != password_confirm) {
        error_label_->setText("Пароли не совпадают");
        error_label_->show();
        return;
    }

    error_label_->hide();
    register_button_->setEnabled(false);
    register_button_->setText("Регистрация...");

    auto result = grpc_client_->registerUser(username.toStdString(), password.toStdString());

    register_button_->setEnabled(true);
    register_button_->setText("Зарегистрироваться");

    if (result.success) {
        QMessageBox::information(this, "Успех", "Регистрация прошла успешно!");
        accept();
    } else {
        error_label_->setText(QString::fromStdString(result.message));
        error_label_->show();
    }
}
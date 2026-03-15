#include "BoardWidgets.hpp"

#include <QLabel>
#include <QFormLayout>
#include <QPushButton>
#include <QPoint>
#include <QVBoxLayout>
#include <QHBoxLayout>

Widget::Widget(uint64_t widget_id, BoardScreen* board, QWidget* parent)
    : QDialog(parent)
    , widget_id_(widget_id)
    , board_(board) {
    SetupUI();
}

void Widget::SetupUI() {

    UpdateCoords(x(), y());

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);

    QLabel* title_label = new QLabel(QString::fromStdString(std::to_string(widget_id_)), this);
    title_label->setAlignment(Qt::AlignCenter);
    QFont title_font = title_label->font();
    title_font.setPointSize(10);
    title_font.setBold(true);
    title_label->setFont(title_font);
    layout->addWidget(title_label);

    QFormLayout* form_layout = new QFormLayout();
    form_layout->setSpacing(10);

    QLineEdit* x_setup_line_ = new QLineEdit(this);
    x_setup_line_->setPlaceholderText("x");
    x_setup_line_->setMinimumHeight(20);
    form_layout->addRow("x:", x_setup_line_);

    
    QLineEdit* y_setup_line_ = new QLineEdit(this);
    y_setup_line_->setPlaceholderText("y");
    y_setup_line_->setMinimumHeight(20);
    form_layout->addRow("y:", y_setup_line_);

    layout->addLayout(form_layout);

    QHBoxLayout* button_layout = new QHBoxLayout();

    QPushButton* update_button_ = new QPushButton("Обновить", this);
    update_button_->setMinimumHeight(20);
    button_layout->addWidget(update_button_);

    QPushButton* delete_button_ = new QPushButton("Удалить", this);
    delete_button_->setMinimumHeight(20);
    button_layout->addWidget(delete_button_);

    layout->addLayout(button_layout);

    connect(update_button_, &QPushButton::clicked, this, &Widget::onUpdateClicled);
    connect(delete_button_, &QPushButton::clicked, this, &Widget::onDeleteClicked);

}

void Widget::UpdateCoords(int new_x, int new_y) {
    x_ = new_x;
    y_ = new_y;
    std::string new_label = "x=" + std::to_string(x_) + " y=" + std::to_string(y_);
    setWindowTitle(QString::fromStdString(new_label));
}

void Widget::onUpdateClicled() {
    QString x = x_setup_line_->text();
    QString y = y_setup_line_->text();

    if (x.isEmpty() || y.isEmpty()) {
        return;
    }

    int new_x = std::stoi(x.toStdString());
    int new_y = std::stoi(y.toStdString());

    emit requestUpdate(widget_id_, {new_x, new_y});
}

void Widget::onDeleteClicked() {

    emit requestDelete(widget_id_);
}

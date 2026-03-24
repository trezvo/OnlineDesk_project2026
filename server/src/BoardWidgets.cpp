#include "BoardWidgets.hpp"
#include <QListWidgetItem>

namespace widgets {

BoardsListWidget::BoardsListWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void BoardsListWidget::setupUI() {
    setStyleSheet(
        "QWidget { background-color: #f5f5f5; }"
        "QListWidget {"
        "   background-color: white;"
        "   border: 1px solid #ddd;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "}"
        "QListWidget::item {"
        "   padding: 10px;"
        "   border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #e3f2fd;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #2196f3;"
        "   color: white;"
        "}"
        "QPushButton {"
        "   background-color: #4caf50;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 16px;"
        "   border-radius: 4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QLineEdit {"
        "   border: 1px solid #ccc;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QLineEdit:focus {"
        "   border-color: #2196f3;"
        "}"
    );
    
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    QLabel* title = new QLabel("Мои доски");
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    QHBoxLayout* createLayout = new QHBoxLayout();
    boardNameEdit = new QLineEdit();
    boardNameEdit->setPlaceholderText("Название доски...");
  
    createBtn = new QPushButton("Создать");
    createLayout->addWidget(boardNameEdit);
    createLayout->addWidget(createBtn);
    mainLayout->addLayout(createLayout);
  
    boardsList = new QListWidget();
    boardsList->setMinimumHeight(300);
    mainLayout->addWidget(boardsList);
    
    connect(boardsList, &QListWidget::itemDoubleClicked, 
            this, &BoardsListWidget::onItemDoubleClicked);
    connect(createBtn, &QPushButton::clicked, 
            this, &BoardsListWidget::onCreateClicked);
}

void BoardsListWidget::addBoard(uint64_t boardId, const QString& boardName) {
    QListWidgetItem* item = new QListWidgetItem(boardName);
    item->setData(Qt::UserRole, QVariant::fromValue(boardId));
    boardsList->addItem(item);
}

void BoardsListWidget::clear() {
    boardsList->clear();
    boardNameEdit->clear();
}

void BoardsListWidget::onItemDoubleClicked(QListWidgetItem* item) {
    uint64_t boardId = item->data(Qt::UserRole).value<uint64_t>();
    if (boardId != 0) {
        emit boardSelected(boardId);
    }
}

void BoardsListWidget::onCreateClicked() {
    QString boardName = boardNameEdit->text().trimmed();
    if (boardName.isEmpty()) {
        return;
    }
    emit createBoardRequested(boardName);
    boardNameEdit->clear();
}

}

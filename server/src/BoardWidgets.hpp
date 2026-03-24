#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>

namespace widgets {

class BoardsListWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit BoardsListWidget(QWidget* parent = nullptr);
    
    void addBoard(uint64_t boardId, const QString& boardName);
    void clear();
    
signals:
    void boardSelected(uint64_t boardId);
    void createBoardRequested(const QString& boardName);
    
private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onCreateClicked();
    
private:
    void setupUI();
    
    QVBoxLayout* mainLayout;
    QListWidget* boardsList;
    QPushButton* createBtn;
    QLineEdit* boardNameEdit;
};

}

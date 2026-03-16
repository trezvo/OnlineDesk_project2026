#pragma once

#include "AppControllerFwd.hpp"
#include "GrpcBoardClientFwd.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <string>
#include <memory>

class BoardButton : public QPushButton {

    Q_OBJECT

    uint64_t board_id_;

signals:
    void onDoubleClickedBoardButton(uint64_t board_id);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

public:
    explicit BoardButton(const QString& board_name, uint64_t board_id, QWidget* parent);
    ~BoardButton() = default;
};


class BoardsButtonList : public QWidget {

    Q_OBJECT

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    AppController& app_;
    QVector<BoardButton*> buttons_;
    QVBoxLayout* layout_;
    QPushButton* refresh_button_;

    void clear();

public:

    explicit BoardsButtonList(std::shared_ptr<GrpcBoardClient> grpc_client, AppController& app, QWidget* parent = nullptr);
    ~BoardsButtonList() = default;
    void UpdateUI();

};
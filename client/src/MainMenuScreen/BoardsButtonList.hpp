#pragma once

#include "AppController/AppControllerFwd.hpp"
#include "GrpcClient/GrpcBoardClientFwd.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <string>
#include <memory>

class BoardButton : public QPushButton {

    Q_OBJECT

    uint64_t board_id_;

signals:
    void onDoubleClickedBoardButton(uint64_t board_id);
    void deleteRequested(uint64_t board_id);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

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

private slots:

    void onDeleteButtonClicked(uint64_t board_id);

signals:

    void deleteBoardRequested(uint64_t board_id);

public:

    explicit BoardsButtonList(std::shared_ptr<GrpcBoardClient> grpc_client, AppController& app, QWidget* parent = nullptr);
    ~BoardsButtonList() = default;
    void UpdateUI();
};
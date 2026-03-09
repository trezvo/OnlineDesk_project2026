#pragma once

#include "AppController.hpp"
#include "GrpcBoardClient.hpp"
#include <QListWidget>
#include <QPushButton>
#include <memory>


class BoardButton : public QPushButton {

    uint32_t board_id_;
    std::shared_ptr<AppController> app_;

    void SetupUI(const Qstring& board_name);

private signals:
    void doubleClickedBoardButton(uint32_t board_id);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

public:
    explicit BoardButton(const QString& board_name, uint32_t board_id, std::shared_ptr<AppController> app, QObject* parent);
    ~BoardButton();
};


class BoardsListWidget : public QListWidget {

    Q_OBJECT

    

public slots:
    void processUpdateUI() {
        UpdateUI();
    }

public:

    explicit BoardsListWidget(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent);
    ~BoardsListWidget();

    void UpdateUI();

};
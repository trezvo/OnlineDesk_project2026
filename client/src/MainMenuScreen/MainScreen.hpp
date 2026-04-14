#pragma once
#include "GrpcClient/GrpcBoardClient.hpp"
#include "AppController/AppControllerFwd.hpp"
#include "BoardsButtonList.hpp"
#include <QMainWindow>
#include <QHBoxLayout>
#include <memory> 
#include <vector>

class MainScreen : public QMainWindow {

    Q_OBJECT

    void SetupUI();

    std::shared_ptr<GrpcBoardClient> grpc_client_;
    AppController& app_;
    BoardsButtonList* boards_list_;
    QHBoxLayout* layout_;
    QLineEdit* lobby_id_line_;
    QPushButton* lobby_join_;
    QLineEdit* rename_id_line_;
    QPushButton* rename_board_;

    // public slots:
    //     void onBoardCreateClicked();
    //     void onBoardJoinClicked();
    //TODO

public:

    explicit MainScreen(std::shared_ptr<GrpcBoardClient> grpc_client, AppController& app, QWidget* parent = nullptr);


signals:
    void onMainScreenFinished(uint64_t board_id);

private slots:

    void onCreateBoardClicked();
    void onJoinPartyClicked();
    void onRenameBoardClicked();

};
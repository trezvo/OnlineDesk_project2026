#pragma once

#include "GrpcBoardClient.hpp"
#include "AuthDialog.hpp"
#include "MainScreen.hpp"
#include "BoardScreen.hpp"
#include <QDialog>
#include <QScopedPointer>
#include <QMainWindow>
#include <memory>

class AppController : public QObject {

    Q_OBJECT

    const std::shared_ptr<GrpcBoardClient> grpc_client_;
    AuthDialog* auth_dialog_;
    MainScreen* main_screen_;
    BoardScreen* board_screen_;

    void showAuthDialog();
    void showMainScreen();
    void showBoardScreen(uint64_t board_id);

public slots:
    void onAuthDialogFinished();
    void onMainScreenFinished(uint64_t board_id);
    // void onBoardScreenFinished();

public:

    explicit AppController(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* parent = nullptr);
    ~AppController() override {}

    void run();
};

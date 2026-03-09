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
    QScopedPointer<AuthDialog> auth_dialog_;
    QScopedPointer<MainScreen> main_screen_;
    QScopedPointer<BoardScreen> board_screen_;

    void showAuthDialog();
    void showMainScreen();
    void showBoardScreen(uint32_t board_id);

private slots:
    void onAuthDialogFineshed();
    void onMainScreenFinished(uint32_t board_id);
    void onBoardScreenFinished();

public:

    explicit AppController(std::shared_ptr<GrpcBoardClient> grpc_client, QObject* paretn = nullptr);
    ~AppController() {}

    void run();
};

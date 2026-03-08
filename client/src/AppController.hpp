#pragma once

#include "GrpcBoardClient.hpp"
#include "AuthDialog.hpp"
#include "MainScreen.hpp"
#include <QDialog>
#include <QMainWindow>
#include <memory>

class AppController {

    const std::shared_ptr<GrpcBoardClient> grpc_client_;
    std::unique_ptr<AuthDialog> auth_dialog_ = nullptr;
    std::unique_ptr<QMainWindow> main_window_ = nullptr;
    
public:

    explicit AppController(std::shared_ptr<GrpcBoardClient> grpc_client) : grpc_client_(grpc_client) {};

    void run();
};

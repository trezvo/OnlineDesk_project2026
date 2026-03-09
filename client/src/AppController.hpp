#pragma once

#include "GrpcBoardClient.hpp"
#include "AuthDialog.hpp"
#include "MainScreen.hpp"
#include <QDialog>
#include <QMainWindow>
#include <memory>

class AppController {

    const std::shared_ptr<GrpcBoardClient> grpc_client_;
    AuthDialog* auth_dialog_ = nullptr;
    QMainWindow* main_window_ = nullptr;
    
public:

    explicit AppController(std::shared_ptr<GrpcBoardClient> grpc_client) : grpc_client_(grpc_client) {};

    void run();
};

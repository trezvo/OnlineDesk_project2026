#include <QApplication>
#include <QProcessEnvironment>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "GrpcClient/GrpcBoardClient.hpp"
#include "AppController/AppController.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
    QApplication app(argc, argv);

    auto client = std::make_shared<GrpcBoardClient>("127.0.0.1:50051");
    
    AppController controller(client);
    controller.run();

    // std::cout << "controller runs" << std::endl;

    return app.exec();
}
#include <QApplication>
#include <QProcessEnvironment>
#include <memory>
#include "GrpcClient/GrpcBoardClient.hpp"
#include "AppController/AppController.hpp"

int main(int argc, char *argv[]) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
    QApplication app(argc, argv);
    
    auto client = std::make_shared<GrpcBoardClient>("localhost:50051");
    
    AppController controller(client);
    controller.run();

    return app.exec();
}
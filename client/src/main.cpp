#include <QApplication>
#include <QProcessEnvironment>
#include <memory>
#include "AuthDialog.h"
#include "RegisterDialog.h"
#include "GrpcBoardClient.h"

int main(int argc, char *argv[]) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }

    QApplication app(argc, argv);
    
    auto client = std::make_shared<GrpcBoardClient>("localhost:50051");
    
    AuthDialog *login = new AuthDialog(client);
    login->show();
    
    QObject::connect(login, &AuthDialog::registerRequested, [client]() {
        RegisterDialog *reg = new RegisterDialog(client);
        reg->show();
    });
    
    return app.exec();
}
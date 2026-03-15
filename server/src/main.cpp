#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <csignal>
#include <memory>
#include "AuthenticationImpl.hpp"
#include "BoardImpl.hpp"

std::unique_ptr<grpc::Server> g_server;

void stopServer(int) {
    if (g_server) {
        g_server->Shutdown();
    }
}

void runServer(const std::string &server_address) {
    auto authentication_service =
        std::make_shared<auth_module::AuthenticationServiceImpl>();
    auto board_service =
        std::make_shared<board_module::BoardServiceImpl>(authentication_service
        );

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(authentication_service.get());
    builder.RegisterService(board_service.get());

    g_server = builder.BuildAndStart();

    if (!g_server) {
        std::cerr << "Server starting gone wrong \n";
        return;
    }

    g_server->Wait();
}

int main(int argc, char *argv[]) {
    std::signal(SIGINT, stopServer);
    std::signal(SIGTERM, stopServer);

    std::string my_address = "0.0.0.0:50051";

    if (argc > 1) {
        my_address = argv[1];
    }

    runServer(my_address);

    return 0;
}
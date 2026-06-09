#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <csignal>
#include <memory>
#include <iostream>
#include "AuthenticationImpl/AuthenticationImpl.hpp"
#include "BoardImpl/BoardImpl.hpp"
#include "Database/ConnectionManager.hpp"

std::unique_ptr<grpc::Server> g_server;

void stopServer(int) {
    if (g_server) {
        g_server->Shutdown();
    }
}

void runServer(const std::string &server_address) {

    const ODBConnectionManager::ConnectionConfig conf{
        "desk_admin",
        "desk_admin",
        "desk_db",
        "localhost",
        5432
    };

    auto database_connection_pool = std::make_shared<ODBConnectionManager>(20, conf);

    auto authentication_service =
        std::make_shared<auth_module::AuthenticationServiceImpl>(database_connection_pool);
    auto board_service =
        std::make_shared<board_module::BoardServiceImpl>(
            authentication_service, 
            database_connection_pool
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

    std::cout << "server started" << std::endl;

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
#pragma once

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"

struct LoginResult {
    bool success;
    std::string message;
    uint64_t user_id = 0;
    uint64_t user_token = 0;
};

struct RegisterResult {
    bool success;
    std::string message;
};

class GrpcBoardClient {
public:
    GrpcBoardClient(const std::string& server_address);
    
    LoginResult login(const std::string& username, const std::string& password);
    RegisterResult registerUser
    (const std::string& username, const std::string& password);

private:
    std::unique_ptr<online_desk::auth::AuthenticationService::Stub> auth_stub_;
};
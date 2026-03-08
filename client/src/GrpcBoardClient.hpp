#pragma once

#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"

struct LoginMetaData {
    bool success = false;
    std::string message = "";
    std::string user_id = "";
    uint64_t user_token = 0;
};

struct RegisterResult {
    bool success = false;
    std::string message = "";
};

class GrpcBoardClient {
public:

    explicit GrpcBoardClient(const std::string& server_address);
    
    void login(const std::string& username, const std::string& password);
    RegisterResult registerUser
    (const std::string& username, const std::string& password);
    const LoginMetaData& get_login_data() const {
        return login_data_;
    }

private:
    
    LoginMetaData login_data_;

    std::unique_ptr<online_desk::auth::AuthenticationService::Stub> auth_stub_;
};
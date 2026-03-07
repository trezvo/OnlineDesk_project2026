#include "GrpcBoardClient.hpp"
#include <iostream>

GrpcBoardClient::GrpcBoardClient(const std::string& server_address) {
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auth_stub_ = online_desk::auth::AuthenticationService::NewStub(channel);
}

LoginResult GrpcBoardClient::login(const std::string& username, const std::string& password) {
    online_desk::auth::LoginRequest request;
    request.set_username(username);
    request.set_password(password);
    
    online_desk::auth::LoginResponse response;
    grpc::ClientContext context;
    
    grpc::Status status = auth_stub_->UserLogin(&context, request, &response);
    
    if (status.ok() && response.login_succeed()) {
        return {true, response.message(), response.user_id(), response.user_token()};
    } else {
        std::string error_msg = status.ok() ? response.message() : status.error_message();
        return {false, error_msg};
    }
}

RegisterResult GrpcBoardClient::registerUser(const std::string& username, const std::string& password) {
    online_desk::auth::RegisterRequest request;
    request.set_username(username);
    request.set_password(password);
    
    online_desk::auth::RegisterResponse response;
    grpc::ClientContext context;
    grpc::Status status = auth_stub_->UserRegister(&context, request, &response);
    
    if (status.ok() && response.register_succeed()) {
        return {true, response.message()};
    } else {
        std::string error_msg = status.ok() ? response.message() : status.error_message();
        return {false, error_msg};
    }
}
#include "GrpcBoardClient.hpp"
#include <iostream>
#include <vector>
#include <utility>

GrpcBoardClient::GrpcBoardClient(const std::string& server_address) {
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auth_stub_ = online_desk::auth::AuthenticationService::NewStub(channel);
    board_stub_ = online_desk::board::BoardService::NewStub(channel);
}

void GrpcBoardClient::login(const std::string& username, const std::string& password) {
    online_desk::auth::LoginRequest request;
    request.set_username(username);
    request.set_password(password);
    
    online_desk::auth::LoginResponse response;
    grpc::ClientContext context;
    
    grpc::Status status;

    status = auth_stub_->UserLogin(&context, request, &response);
    
    if (status.ok() && response.login_succeed()) {

        login_data_ = {true, response.message(), response.user_id(), response.user_token()};
    } else {
        std::string error_msg = status.ok() ? response.message() : status.error_message();
        login_data_ = {false, std::move(error_msg)};
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

std::pair<bool, std::vector<BoardInfo>> GrpcBoardClient::fetchUserBoards() {
    online_desk::board::FetchUserBoardsRequest request;
    request.set_user_id(login_data_.user_id);
    request.set_user_token(login_data_.user_token);

    online_desk::board::FetchUserBoardsResponse response;
    grpc::ClientContext context;
    grpc::Status status = board_stub_->FetchUserBoards(&context, request, &response);

    if (!status.ok() || !response.success()) {
        return {false, {}};
    }

    std::vector<BoardInfo> boards_vec;

    for (int i = 0; i < response.boards_size(); i++) {
        const auto& board_info = response.boards(i);
        boards_vec.push_back({board_info.board_id(), board_info.board_name()});
    }
    
    return {true, boards_vec};
}
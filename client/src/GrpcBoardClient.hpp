#pragma once

#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "board.grpc.pb.h"
#include "BoardScreenFwd.hpp"
#include "BoardWorkerInterface.hpp"
#include "SessionReactor.hpp"

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

struct BoardInfoInternal {
    uint64_t board_id = 0;
    std::string board_name = "";
};

struct CreateBoardResult{
    bool success = false;
    std::string message = "";
    uint64_t board_id = 0;
};


using namespace online_desk::auth;
using namespace online_desk::board;

class GrpcBoardClient {
public:

    explicit GrpcBoardClient(const std::string& server_address);
    
    void login(const std::string& username, const std::string& password);
    RegisterResult registerUser
    (const std::string& username, const std::string& password);
    std::pair<bool, std::vector<BoardInfoInternal>> fetchUserBoards();
    
    const LoginMetaData& get_login_data() const {
        return login_data_;
    }
    const uint64_t GetUserToken() const {
        return login_data_.user_token;
    }


    CreateBoardResult createBoard(const std::string& board_name);
    SessionReactorInterface* connectToBoard(BoardWorkerInterface& worker, uint64_t board_id);

private:
    
    std::unique_ptr<online_desk::auth::AuthenticationService::Stub> auth_stub_;
    std::unique_ptr<online_desk::board::BoardService::Stub> board_stub_;

    LoginMetaData login_data_;

};

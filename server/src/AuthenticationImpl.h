#pragma once

#include "sodium.h"
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "auth.pb.h"

#include <vector>
#include <map>
#include <string>

namespace auth_module {

namespace contract = online_desk::auth;

class UsersDataBase {

public:

    struct UserInfo {
        uint64_t id;
        std::string username;
        std::string password;
    };

    UserInfo GetUserData(const std::string& name);
    void SetUserData(uint64_t id, const std::string& username, const std::string& password);
    bool contains(const std::string& name) {
        return users_data_base_.contains(name);
    }

private: 

    std::map<std::string, UserInfo> users_data_base_;

};


class AuthenticationServiceImpl final : public contract::AuthenticationService::Service {

    uint64_t users_counter_;
    std::shared_ptr<UsersDataBase> users_data_base_;    

    std::string HashPassword(const std::string& password);

    bool VerifyPassword(const std::string& password, const std::string& hash);

public:

    AuthenticationServiceImpl();
    ~AuthenticationServiceImpl() override;

    grpc::Status UserRegister (
        grpc::ServerContext* context,
        const contract::RegisterRequest* request,
        contract::RegisterResponse* response
    ) override;

    grpc::Status UserLogin (
        grpc::ServerContext* context,
        const contract::LoginRequest* request,
        contract::LoginResponse* response
    ) override;
};

}
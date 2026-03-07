#pragma once

#include "sodium.h"
#include <uuid/uuid.h>
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "auth.pb.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>

namespace auth_module {

namespace contract = online_desk::auth;

class UsersDataBase {

public:

    struct UserInfo {
        std::string id;
        std::string username;
        std::string password;
    };

    UserInfo GetUserData(const std::string& name);
    void SetUserData(const std::string& id, const std::string& username, const std::string& password);
    bool contains(const std::string& name) {
        return users_data_base_.contains(name);
    }

private: 

    std::map<std::string, UserInfo> users_data_base_;

};


class AuthenticationServiceImpl final : public contract::AuthenticationService::Service {

    uint64_t users_counter_;
    std::shared_ptr<UsersDataBase> users_data_base_;    

    static std::string GenerateUUID();
    static std::string HashPassword(const std::string& password);
    
    static bool VerifyPassword(const std::string& password, const std::string& hash);

    std::mutex token_mutex_;
    std::unordered_map<std::string, uint64_t> online_tokens_;
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

    inline const bool ValidateUserToken(std::string user_id, uint64_t user_token) const {
        auto token_it = online_tokens_.find(user_id);
        return token_it != online_tokens_.end() && (*token_it).second == user_token;
    }
};

}
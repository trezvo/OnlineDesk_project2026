#pragma once

#include <grpcpp/grpcpp.h>
#include <uuid/uuid.h>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "Database/ConnectionManager.hpp"
#include "Database/UserRepository.hpp"
#include "auth.grpc.pb.h"
#include "auth.pb.h"
#include "sodium.h"

namespace auth_module {

namespace contract = online_desk::auth;

class AuthenticationServiceImpl final
    : public contract::AuthenticationService::Service {

    static std::string GenerateUUID();
    static std::string HashPassword(const std::string &password);

    static bool
    VerifyPassword(const std::string &password, const std::string &hash);

    std::shared_ptr<db::UserRepository> user_table_;
    std::mutex token_mutex_;
    std::unordered_map<std::string, uint64_t> online_tokens_;
    std::mt19937_64 create_rand_64_;

public:
    AuthenticationServiceImpl(std::shared_ptr<ODBConnectionManager> connection_pool);
    ~AuthenticationServiceImpl() override;

    grpc::Status UserRegister(
        grpc::ServerContext *context,
        const contract::RegisterRequest *request,
        contract::RegisterResponse *response
    ) override;

    grpc::Status UserLogin(
        grpc::ServerContext *context,
        const contract::LoginRequest *request,
        contract::LoginResponse *response
    ) override;

    inline const bool
    ValidateUserToken(std::string user_id, uint64_t user_token) const {
        auto token_it = online_tokens_.find(user_id);
        return token_it != online_tokens_.end() &&
               (*token_it).second == user_token;
    }
};

}  // namespace auth_module
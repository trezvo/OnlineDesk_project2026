#include "AuthenticationImpl.hpp"
#include <sodium.h>
#include <uuid/uuid.h>
#include <exception>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <iostream>

namespace auth_module {

namespace contract = online_desk::auth;

AuthenticationServiceImpl::AuthenticationServiceImpl(std::shared_ptr<ODBConnectionManager> connection_pool) :
    user_table_(std::make_shared<db::UserRepository>(connection_pool)) {
}

AuthenticationServiceImpl::~AuthenticationServiceImpl() = default;


std::string AuthenticationServiceImpl::GenerateUUID() {
    uuid_t uuid;
    uuid_generate(uuid);

    char uuid_buffer[37];
    uuid_unparse(uuid, uuid_buffer);

    return std::string(uuid_buffer);
}

std::string AuthenticationServiceImpl::HashPassword(const std::string &password
) {
    static const int sodium_init_code_ = sodium_init();

    if (sodium_init_code_ < 0) {
        return password;
    }

    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hash, password.c_str(), password.length(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE
        ) != 0) {
        throw std::runtime_error("Out of memory");
    }

    return std::string(hash);
}

bool AuthenticationServiceImpl::VerifyPassword(
    const std::string &password,
    const std::string &hash
) {
    return crypto_pwhash_str_verify(
               hash.c_str(), password.c_str(), password.length()
           ) == 0;
}

grpc::Status AuthenticationServiceImpl::UserRegister(
    grpc::ServerContext *server,
    const contract::RegisterRequest *request,
    contract::RegisterResponse *response
) {
    std::cout << "here_register1" << std::endl;

        try {
        const std::string &username = request->username();
        const std::string &password = request->password();

        if (username.empty() || password.empty()) {
            response->set_register_succeed(false);
            response->set_message("Введите логин и пароль");
            return grpc::Status::OK;
        }

        db::User user(GenerateUUID(), username, HashPassword(password));

        response->set_register_succeed(true);
        response->set_message("Успешная регистрация");

        if (!user_table_->create(user)) {    
            response->set_register_succeed(false);
            response->set_message("Пользователь с данным именем уже существует!");
        }

        std::cout << "regisret_out_here1" << std::endl;

        return grpc::Status::OK;
    }
    catch (const odb::exception& e) {
        std::cerr << "ODB error: " << e.what() << std::endl;
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    } catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << std::endl;
        return grpc::Status(grpc::StatusCode::UNKNOWN, e.what());
    }
}

grpc::Status AuthenticationServiceImpl::UserLogin(
    grpc::ServerContext *context,
    const contract::LoginRequest *request,
    contract::LoginResponse *response
) {
    static const int sodium_init_code_ = sodium_init();

    const std::string &username = request->username();
    const std::string &password = request->password();

    if (username.empty() || password.empty()) {
        response->set_login_succeed(false);
        response->set_message("Введите логин и пароль");
        return grpc::Status::OK;
    }

    try {
        auto user_query = user_table_->FindByName(username);

        if (!user_query) {
            response->set_login_succeed(false);
            response->set_message("Неверное имя или пароль");
            return grpc::Status::OK;
        }

        db::User user = user_query.value();

        if (!VerifyPassword(password, user.password())) {
            response->set_login_succeed(false);
            response->set_message("Неверное имя или пароль");
            return grpc::Status::OK;
        }

        response->set_login_succeed(true);
        response->set_message("Добро пожаловать!");
        response->set_user_id(user.uuid());

        uint64_t new_login_token = 0;

        if (sodium_init_code_ < 0) {
            new_login_token = create_rand_64_();
        } else {
            new_login_token = ((uint64_t)randombytes_random() << 32) |
                                randombytes_random();
        }
        response->set_user_token(new_login_token);

        {
            std::lock_guard tmp_lock(token_mutex_);
            online_tokens_[user.uuid()] = new_login_token;
        }

    } catch (const odb::exception& e) {
        response->set_login_succeed(false);
        response->set_message(e.what());
        return grpc::Status::OK;
    }

    return grpc::Status::OK;
}

}  // namespace auth_module
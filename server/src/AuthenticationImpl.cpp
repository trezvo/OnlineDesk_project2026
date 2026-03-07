#include "AuthenticationImpl.h"
#include <sodium.h>
#include <string>
#include <map>
#include <exception>

namespace auth_module {

namespace contract = online_desk::auth;

UsersDataBase::UserInfo UsersDataBase::GetUserData(const std::string& name) {
    if (!users_data_base_.contains(name)) {
        throw std::runtime_error("User doesn't exist");
    }
    return users_data_base_.at(name);
}

void UsersDataBase::SetUserData(
    uint64_t id, 
    const std::string& username, 
    const std::string& password
) {
    if (users_data_base_.contains(username)) {
        throw std::runtime_error("User with given name already exists");
    }

    users_data_base_[username] = {id, username, password};
}

std::string AuthenticationServiceImpl::HashPassword(const std::string& password) {
    static int sodium_init_code = sodium_init();

    if (sodium_init_code < 0) {
        return password;
    }

    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
        hash, 
        password.c_str(), 
        password.length(), 
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE
    ) != 0) {
        throw std::runtime_error("Out of memory");
    }
    
    return std::string(hash);
}

bool AuthenticationServiceImpl::VerifyPassword(const std::string& password, const std::string& hash) {
    return crypto_pwhash_str_verify(
        hash.c_str(),
        password.c_str(),
        password.length()
    ) == 0;
}

AuthenticationServiceImpl::AuthenticationServiceImpl() {
    users_counter_ = 1;
    users_data_base_ = std::make_shared<UsersDataBase>();
}

AuthenticationServiceImpl::~AuthenticationServiceImpl() = default;

grpc::Status AuthenticationServiceImpl::UserRegister(
    grpc::ServerContext* server,
    const contract::RegisterRequest* request,
    contract::RegisterResponse* response
) {

    const std::string& username = request->username();
    const std::string& password = request->password();

    if (username.empty() || password.empty()) {
        response->set_register_succeed(false);
        response->set_message("Введите логин и пароль");
        return grpc::Status::OK;
    }

    try {
        users_data_base_->SetUserData(users_counter_++, username, HashPassword(password));
    }
    catch (std::exception& e) {
        response->set_register_succeed(false);
        response->set_message("Пользователь с данным именем уже существует!");
        return grpc::Status::OK;
    }
    
    response->set_register_succeed(true);
    response->set_message("Успешная регистрация");

    return grpc::Status::OK;
}

grpc::Status AuthenticationServiceImpl::UserLogin(
    grpc::ServerContext* context,
    const contract::LoginRequest* request,
    contract::LoginResponse* response
) {

    const std::string& username = request->username();
    const std::string& password = request->password();

    if (username.empty() || password.empty()) {
        response->set_login_succeed(false);
        response->set_message("Введите логин и пароль");
        return grpc::Status::OK;
    }

    try {
        auto user_info = users_data_base_->GetUserData(username);

        if (VerifyPassword(password, user_info.password)) {
            response->set_login_succeed(true);
            response->set_message("Добро пожаловать!");
            response->set_user_id(user_info.id);
            response->set_user_token(user_info.id);
        }
        else {
            response->set_login_succeed(false);
            response->set_message("Неверное имя или пароль");
        }

    } catch (std::exception& e) {        
        response->set_login_succeed(false);
        response->set_message("Неверное имя или пароль");
        return grpc::Status::OK;
    }

    return grpc::Status::OK;
}

}
#include <stdexcept>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "AuthenticationImpl/AuthenticationImpl.hpp"

namespace {

using auth_module::AuthenticationServiceImpl;
using auth_module::UsersDataBase;
namespace contracts = online_desk::auth;

contracts::RegisterResponse RegisterUser(
    AuthenticationServiceImpl& service,
    const std::string& username,
    const std::string& password
) {
    grpc::ServerContext context;
    contracts::RegisterRequest request;
    contracts::RegisterResponse response;
    request.set_username(username);
    request.set_password(password);

    const grpc::Status status = service.UserRegister(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

contracts::LoginResponse LoginUser(
    AuthenticationServiceImpl& service,
    const std::string& username,
    const std::string& password
) {
    grpc::ServerContext context;
    contracts::LoginRequest request;
    contracts::LoginResponse response;
    request.set_username(username);
    request.set_password(password);

    const grpc::Status status = service.UserLogin(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

}  // namespace

TEST(UsersDataBaseTest, SetAndGetReturnsUserData) {
    UsersDataBase db;

    db.SetUserData("user-id-1", "alice", "password-hash");

    ASSERT_TRUE(db.contains("alice"));
    const UsersDataBase::UserInfo user = db.GetUserData("alice");
    EXPECT_EQ(user.id, "user-id-1");
    EXPECT_EQ(user.username, "alice");
    EXPECT_EQ(user.password, "password-hash");
}

TEST(UsersDataBaseTest, ReturnsFalseForUnknownUser) {
    UsersDataBase db;

    EXPECT_FALSE(db.contains("missing-user"));
}

TEST(UsersDataBaseTest, SetUserDataRejectsUsedUsername) {
    UsersDataBase db;
    db.SetUserData("user-id-1", "alice", "password-hash");

    EXPECT_THROW(
        db.SetUserData("user-id-2", "alice", "other-password-hash"),
        std::runtime_error
    );
}

TEST(UsersDataBaseTest, GetUserDataThrowsForUnknownUser) {
    UsersDataBase db;

    EXPECT_THROW(db.GetUserData("missing-user"), std::runtime_error);
}

TEST(AuthenticationServiceImplTest, RegisterRejectsEmptyUsername) {
    AuthenticationServiceImpl service;

    const contracts::RegisterResponse response = RegisterUser(service, "", "secret");

    EXPECT_FALSE(response.register_succeed());
    EXPECT_FALSE(response.message().empty());
}

TEST(AuthenticationServiceImplTest, RegisterRejectsEmptyPassword) {
    AuthenticationServiceImpl service;

    const contracts::RegisterResponse response = RegisterUser(service, "alice", "");

    EXPECT_FALSE(response.register_succeed());
    EXPECT_FALSE(response.message().empty());
}

TEST(AuthenticationServiceImplTest, LoginRejectsEmptyCredentials) {
    AuthenticationServiceImpl service;

    const contracts::LoginResponse response = LoginUser(service, "", "");

    EXPECT_FALSE(response.login_succeed());
    EXPECT_FALSE(response.message().empty());
}

TEST(AuthenticationServiceImplTest, LoginRejectsUnknownUser) {
    AuthenticationServiceImpl service;

    const contracts::LoginResponse response =
        LoginUser(service, "missing-user", "secret");

    EXPECT_FALSE(response.login_succeed());
    EXPECT_FALSE(response.message().empty());
}

TEST(AuthenticationServiceImplTest, RegisterRejectsUsedUsername) {
    AuthenticationServiceImpl service;

    const contracts::RegisterResponse first_response =
        RegisterUser(service, "alice", "secret");
    const contracts::RegisterResponse second_response =
        RegisterUser(service, "alice", "another-secret");

    EXPECT_TRUE(first_response.register_succeed());
    EXPECT_FALSE(second_response.register_succeed());
    EXPECT_FALSE(second_response.message().empty());
}

TEST(AuthenticationServiceImplTest, LoginRejectsWrongPassword) {
    AuthenticationServiceImpl service;
    ASSERT_TRUE(RegisterUser(service, "alice", "secret").register_succeed());

    const contracts::LoginResponse response = LoginUser(service, "alice", "wrong");

    EXPECT_FALSE(response.login_succeed());
    EXPECT_FALSE(response.message().empty());
}

TEST(AuthenticationServiceImplTest, ValidateUserTokenReturnsFalseForUnknownUserId) {
    AuthenticationServiceImpl service;

    EXPECT_FALSE(service.ValidateUserToken("missing-user-id", 123));
}

TEST(AuthenticationServiceImplTest, LoginExistingUserReturnsValidToken) {
    AuthenticationServiceImpl service;
    ASSERT_TRUE(RegisterUser(service, "alice", "secret").register_succeed());

    const contracts::LoginResponse response = LoginUser(service, "alice", "secret");

    ASSERT_TRUE(response.login_succeed());
    ASSERT_FALSE(response.user_id().empty());
    EXPECT_TRUE(service.ValidateUserToken(response.user_id(), response.user_token()));
    EXPECT_FALSE(service.ValidateUserToken(response.user_id(), response.user_token() + 1));
}

TEST(AuthenticationServiceImplTest, FailedLoginDoesNotInvalidateExistingToken) {
    AuthenticationServiceImpl service;
    ASSERT_TRUE(RegisterUser(service, "alice", "secret").register_succeed());

    const contracts::LoginResponse success_response =
        LoginUser(service, "alice", "secret");
    const contracts::LoginResponse failed_response =
        LoginUser(service, "alice", "wrong-password");

    ASSERT_TRUE(success_response.login_succeed());
    EXPECT_FALSE(failed_response.login_succeed());
    EXPECT_TRUE(service.ValidateUserToken(
        success_response.user_id(),
        success_response.user_token()
    ));
}

TEST(AuthenticationServiceImplTest, TwoUsersCanHoldValidTokensAtTheSameTime) {
    AuthenticationServiceImpl service;
    ASSERT_TRUE(RegisterUser(service, "alice", "alice-secret").register_succeed());
    ASSERT_TRUE(RegisterUser(service, "bob", "bob-secret").register_succeed());

    const contracts::LoginResponse alice_response =
        LoginUser(service, "alice", "alice-secret");
    const contracts::LoginResponse bob_response =
        LoginUser(service, "bob", "bob-secret");

    ASSERT_TRUE(alice_response.login_succeed());
    ASSERT_TRUE(bob_response.login_succeed());
    ASSERT_FALSE(alice_response.user_id().empty());
    ASSERT_FALSE(bob_response.user_id().empty());
    EXPECT_NE(alice_response.user_id(), bob_response.user_id());
    EXPECT_TRUE(service.ValidateUserToken(
        alice_response.user_id(),
        alice_response.user_token()
    ));
    EXPECT_TRUE(service.ValidateUserToken(
        bob_response.user_id(),
        bob_response.user_token()
    ));
}

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "AuthenticationImpl/AuthenticationImpl.hpp"
#include "BoardImpl/BoardImpl.hpp"

namespace {

namespace auth_contracts = online_desk::auth;
namespace board_contracts = online_desk::board;

struct LoggedInUser {
    std::string user_id;
    uint64_t token;
};

LoggedInUser RegisterAndLogin(
    auth_module::AuthenticationServiceImpl& auth_service,
    const std::string& username,
    const std::string& password
) {
    grpc::ServerContext register_context;
    auth_contracts::RegisterRequest register_request;
    auth_contracts::RegisterResponse register_response;
    register_request.set_username(username);
    register_request.set_password(password);

    const grpc::Status register_status =
        auth_service.UserRegister(&register_context, &register_request, &register_response);
    EXPECT_TRUE(register_status.ok());
    EXPECT_TRUE(register_response.register_succeed());

    grpc::ServerContext login_context;
    auth_contracts::LoginRequest login_request;
    auth_contracts::LoginResponse login_response;
    login_request.set_username(username);
    login_request.set_password(password);

    const grpc::Status login_status =
        auth_service.UserLogin(&login_context, &login_request, &login_response);
    EXPECT_TRUE(login_status.ok());
    EXPECT_TRUE(login_response.login_succeed());

    return {login_response.user_id(), login_response.user_token()};
}

board_contracts::CreateBoardResponse CreateBoard(
    board_module::BoardServiceImpl& board_service,
    const LoggedInUser& user,
    const std::string& board_name
) {
    grpc::ServerContext context;
    board_contracts::CreateBoardRequest request;
    board_contracts::CreateBoardResponse response;
    request.set_user_id(user.user_id);
    request.set_user_token(user.token);
    request.set_board_name(board_name);

    const grpc::Status status = board_service.CreateBoard(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

board_contracts::FetchUserBoardsResponse FetchUserBoards(
    board_module::BoardServiceImpl& board_service,
    const LoggedInUser& user
) {
    grpc::ServerContext context;
    board_contracts::FetchUserBoardsRequest request;
    board_contracts::FetchUserBoardsResponse response;
    request.set_user_id(user.user_id);
    request.set_user_token(user.token);

    const grpc::Status status = board_service.FetchUserBoards(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

board_contracts::RenameBoardResponse RenameBoard(
    board_module::BoardServiceImpl& board_service,
    const LoggedInUser& user,
    uint64_t board_id,
    const std::string& new_name
) {
    grpc::ServerContext context;
    board_contracts::RenameBoardRequest request;
    board_contracts::RenameBoardResponse response;
    request.set_user_id(user.user_id);
    request.set_user_token(user.token);
    request.set_board_id(board_id);
    request.set_new_board_name(new_name);

    const grpc::Status status = board_service.RenameBoard(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

board_contracts::DeleteBoardResponse DeleteBoard(
    board_module::BoardServiceImpl& board_service,
    const LoggedInUser& user,
    uint64_t board_id
) {
    grpc::ServerContext context;
    board_contracts::DeleteBoardRequest request;
    board_contracts::DeleteBoardResponse response;
    request.set_user_id(user.user_id);
    request.set_user_token(user.token);
    request.set_board_id(board_id);

    const grpc::Status status = board_service.DeleteBoard(&context, &request, &response);
    EXPECT_TRUE(status.ok());
    return response;
}

}  // namespace

TEST(BoardServiceImplTest, LoggedInUserCanCreateAndFetchBoard) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser user = RegisterAndLogin(*auth_service, "alice", "secret");

    const board_contracts::CreateBoardResponse create_response =
        CreateBoard(board_service, user, "board");

    ASSERT_TRUE(create_response.success());

    const board_contracts::FetchUserBoardsResponse fetch_response =
        FetchUserBoards(board_service, user);
    ASSERT_TRUE(fetch_response.success());
    ASSERT_EQ(fetch_response.boards_size(), 1);
    EXPECT_EQ(fetch_response.boards(0).board_id(), create_response.board_id());
    EXPECT_EQ(fetch_response.boards(0).board_name(), "board");
}

TEST(BoardServiceImplTest, CreateBoardRejectsInvalidToken) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    LoggedInUser user = RegisterAndLogin(*auth_service, "alice", "secret");
    user.token += 1;

    const board_contracts::CreateBoardResponse create_response =
        CreateBoard(board_service, user, "board");

    EXPECT_FALSE(create_response.success());
    EXPECT_FALSE(create_response.message().empty());
}

TEST(BoardServiceImplTest, CreateBoardRejectsEmptyName) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser user = RegisterAndLogin(*auth_service, "alice", "secret");

    const board_contracts::CreateBoardResponse create_response =
        CreateBoard(board_service, user, "");

    EXPECT_FALSE(create_response.success());
    EXPECT_FALSE(create_response.message().empty());
    EXPECT_EQ(FetchUserBoards(board_service, user).boards_size(), 0);
}

TEST(BoardServiceImplTest, UsersOnlyFetchTheirOwnBoards) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser alice = RegisterAndLogin(*auth_service, "alice", "alice-secret");
    const LoggedInUser bob = RegisterAndLogin(*auth_service, "bob", "bob-secret");

    const board_contracts::CreateBoardResponse alice_board =
        CreateBoard(board_service, alice, "Alice board");
    const board_contracts::CreateBoardResponse bob_board =
        CreateBoard(board_service, bob, "Bob board");

    ASSERT_TRUE(alice_board.success());
    ASSERT_TRUE(bob_board.success());

    const board_contracts::FetchUserBoardsResponse alice_fetch =
        FetchUserBoards(board_service, alice);
    const board_contracts::FetchUserBoardsResponse bob_fetch =
        FetchUserBoards(board_service, bob);

    ASSERT_EQ(alice_fetch.boards_size(), 1);
    ASSERT_EQ(bob_fetch.boards_size(), 1);
    EXPECT_EQ(alice_fetch.boards(0).board_id(), alice_board.board_id());
    EXPECT_EQ(alice_fetch.boards(0).board_name(), "Alice board");
    EXPECT_EQ(bob_fetch.boards(0).board_id(), bob_board.board_id());
    EXPECT_EQ(bob_fetch.boards(0).board_name(), "Bob board");
}

TEST(BoardServiceImplTest, OwnerCanRenameBoardAndFetchNewName) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser user = RegisterAndLogin(*auth_service, "alice", "secret");
    const board_contracts::CreateBoardResponse board =
        CreateBoard(board_service, user, "Old board");
    ASSERT_TRUE(board.success());

    const board_contracts::RenameBoardResponse rename_response =
        RenameBoard(board_service, user, board.board_id(), "New board");

    ASSERT_TRUE(rename_response.success());

    const board_contracts::FetchUserBoardsResponse fetch_response =
        FetchUserBoards(board_service, user);
    ASSERT_EQ(fetch_response.boards_size(), 1);
    EXPECT_EQ(fetch_response.boards(0).board_name(), "New board");
}

TEST(BoardServiceImplTest, NonOwnerCannotRenameBoard) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser owner = RegisterAndLogin(*auth_service, "owner", "owner-secret");
    const LoggedInUser other = RegisterAndLogin(*auth_service, "other", "other-secret");
    const board_contracts::CreateBoardResponse board =
        CreateBoard(board_service, owner, "Owner board");
    ASSERT_TRUE(board.success());

    const board_contracts::RenameBoardResponse rename_response =
        RenameBoard(board_service, other, board.board_id(), "steal board");

    EXPECT_FALSE(rename_response.success());
    EXPECT_FALSE(rename_response.message().empty());

    const board_contracts::FetchUserBoardsResponse owner_fetch =
        FetchUserBoards(board_service, owner);
    ASSERT_EQ(owner_fetch.boards_size(), 1);
    EXPECT_EQ(owner_fetch.boards(0).board_name(), "Owner board");
}

TEST(BoardServiceImplTest, OwnerCanDeleteBoard) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser user = RegisterAndLogin(*auth_service, "alice", "secret");
    const board_contracts::CreateBoardResponse board =
        CreateBoard(board_service, user, "board");
    ASSERT_TRUE(board.success());

    const board_contracts::DeleteBoardResponse delete_response =
        DeleteBoard(board_service, user, board.board_id());

    ASSERT_TRUE(delete_response.success());
    EXPECT_EQ(FetchUserBoards(board_service, user).boards_size(), 0);
}

TEST(BoardServiceImplTest, NotOwnerCannotDeleteBoard) {
    auto auth_service = std::make_shared<auth_module::AuthenticationServiceImpl>();
    board_module::BoardServiceImpl board_service(auth_service);
    const LoggedInUser owner = RegisterAndLogin(*auth_service, "owner", "owner-secret");
    const LoggedInUser other = RegisterAndLogin(*auth_service, "other", "other-secret");
    const board_contracts::CreateBoardResponse board =
        CreateBoard(board_service, owner, "Owner board");
    ASSERT_TRUE(board.success());

    const board_contracts::DeleteBoardResponse delete_response =
        DeleteBoard(board_service, other, board.board_id());

    EXPECT_FALSE(delete_response.success());
    EXPECT_FALSE(delete_response.message().empty());

    const board_contracts::FetchUserBoardsResponse owner_fetch =
        FetchUserBoards(board_service, owner);
    ASSERT_EQ(owner_fetch.boards_size(), 1);
    EXPECT_EQ(owner_fetch.boards(0).board_id(), board.board_id());
}

find_package(GTest REQUIRED)
include(GoogleTest)

pkg_check_modules(Sodium REQUIRED libsodium)
pkg_check_modules(UUID REQUIRED uuid)

add_executable(online_desk_tests
    auth_tests.cpp
    board_service_tests.cpp
    widgets_db_tests.cpp
    ${PROJECT_SOURCE_DIR}/server/src/AuthenticationImpl/AuthenticationImpl.cpp
    ${PROJECT_SOURCE_DIR}/server/src/BoardImpl/BoardImpl.cpp
    ${PROJECT_SOURCE_DIR}/server/src/SessionImpl/SessionInstance.cpp
    ${PROJECT_SOURCE_DIR}/server/src/SessionImpl/SessionManager.cpp
    ${PROJECT_SOURCE_DIR}/server/src/SessionImpl/SessionReactor.cpp
    ${PROJECT_SOURCE_DIR}/server/src/WidgetsDataBase/WidgetsDB.cpp
)

target_include_directories(online_desk_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/server/src
    ${PROJECT_BINARY_DIR}/generated
    ${Sodium_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(online_desk_tests PRIVATE
    proto_generated
    ${Sodium_LIBRARIES}
    ${UUID_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    GTest::gtest_main
)

target_compile_options(online_desk_tests PRIVATE
    ${Sodium_CFLAGS_OTHER}
)

gtest_discover_tests(online_desk_tests)

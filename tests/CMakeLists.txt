find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(online_desk_tests
    widgets_db_tests.cpp
    ${PROJECT_SOURCE_DIR}/server/src/WidgetsDataBase/WidgetsDB.cpp
)

target_include_directories(online_desk_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/server/src
)

target_link_libraries(online_desk_tests PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(online_desk_tests)

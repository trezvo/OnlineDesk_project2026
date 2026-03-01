set(CLIENT_SOURCES
    src/main.cpp
    src/AuthDialog.cpp
    src/GrpcBoardClient.cpp 
    src/RegisterDialog.cpp
)

add_executable(desk_client ${CLIENT_SOURCES})

target_link_libraries(desk_client PRIVATE
    Qt6::Widgets
    proto_generated
    ${GRPC_LIBRARIES}
    pthread
)

target_include_directories(
    desk_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(desk_client PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)
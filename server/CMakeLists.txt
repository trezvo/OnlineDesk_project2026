set(SERVER_SOURCES
    src/main.cpp
    src/AuthenticationImpl.cpp
    src/AuthenticationImpl.h
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Sodium REQUIRED libsodium)

add_executable(desk_server ${SERVER_SOURCES})

target_link_libraries(desk_server PRIVATE
    ${Sodium_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    proto_generated
)

target_include_directories(desk_server PRIVATE
    ${Sodium_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(desk_server PRIVATE
    ${Sodium_CFLAGS_OTHER}
)

target_compile_definitions(desk_server PRIVATE
    GRPC_USE_REFLECTION
)
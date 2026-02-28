set(SERVER_SOURCES
    src/main.cpp
    src/EchoImpl.cpp
    src/EchoImpl.h
)

add_executable(desk_server ${SERVER_SOURCES})

target_link_libraries(desk_server PRIVATE
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    proto_generated
)

target_include_directories(desk_server PRIVATE
    ${GRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(desk_server PRIVATE
    GRPC_USE_REFLECTION
)
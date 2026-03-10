cmake_minimum_required(VERSION 3.20)
project(OnlineDesk VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED 
    grpc++ 
    grpc
)



find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

find_library(GRPC_REFLECTION_LIB 
    NAMES grpc++_reflection
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

set(PROTO_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/auth.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/board.proto    
)
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

set(PROTO_GENEDATED_SRCS "")

foreach(PROTO_FILE ${PROTO_FILES})

    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_SRCS "${PROTO_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDRS "${PROTO_BINARY_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRCS "${PROTO_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDRS "${PROTO_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")

    add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${PROTO_BINARY_DIR}
            --cpp_out=${PROTO_BINARY_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I${CMAKE_CURRENT_SOURCE_DIR}/proto
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC code from board.proto"
    )

    list(APPEND PROTO_GENERATED_SRCS ${PROTO_SRCS} ${GRPC_SRCS})    
endforeach()

add_library(proto_generated STATIC ${PROTO_GENERATED_SRCS})

add_custom_target(generate_proto_files DEPENDS ${PROTO_GENERATED_SRCS})
add_dependencies(proto_generated generate_proto_files)

target_include_directories(proto_generated PUBLIC
    ${PROTO_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}    
)

target_link_libraries(proto_generated PUBLIC 
    ${GRPC_REFLECTION_LIB}
    ${Protobuf_LIBRARIES}
    ${GRPC_LIBRARIES}
)

add_subdirectory(client)
add_subdirectory(server)
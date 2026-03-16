
file(GLOB HEADERS "src/*.hpp")
list(FILTER HEADERS EXCLUDE REGEX ".*_dev\\.hpp$")

file(GLOB SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*_dev\\.cpp$")

set(SERVER_SOURCES
    ${HEADERS}
    ${SOURCES}
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GRPC REQUIRED 
    grpc++ 
    grpc 
    gpr
)

pkg_check_modules(Sodium REQUIRED libsodium)

pkg_check_modules(UUID REQUIRED uuid)

add_executable(desk_server ${SERVER_SOURCES})

target_link_libraries(desk_server PRIVATE
    proto_generated
    ${Sodium_LIBRARIES}
    ${UUID_LIBRARIES}
    ${GRPC_LIBRARIES}
    ${Protobuf_LIBRARIES}
    pthread
    dl
    rt
)

target_include_directories(desk_server PRIVATE
    ${Sodium_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/../generated
)

target_compile_options(desk_server PRIVATE
    ${Sodium_CFLAGS_OTHER}
)

target_compile_definitions(desk_server PRIVATE
    GRPC_USE_REFLECTION
)
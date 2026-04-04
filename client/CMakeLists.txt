
file(GLOB HEADERS "src/*/*.hpp")
list(FILTER HEADERS EXCLUDE REGEX ".*_dev\\.hpp$")

file(GLOB SOURCES "src/*/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*_dev\\.cpp$")

set(CLIENT_SOURCES
    ${HEADERS}
    ${SOURCES}
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GRPC REQUIRED 
    grpc++ 
    grpc 
    gpr
)

add_executable(desk_client ${CLIENT_SOURCES})

target_link_libraries(desk_client PRIVATE
    Qt6::Widgets
    proto_generated
    ${GRPC_LIBRARIES}
    pthread
    dl
    rt
)

target_include_directories(desk_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GRPC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/../generated
)

set_target_properties(desk_client PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== ВАЖНО: SERVER_ROOT должен указывать на текущую директорию =====
set(SERVER_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Директории для сгенерированных файлов
set(ODB_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/odb_generated)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build/generated)

# ===== ПРОВЕРКА ПУТЕЙ =====
message(STATUS "=========================================")
message(STATUS "Path Debug:")
message(STATUS "  CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  SERVER_ROOT: ${SERVER_ROOT}")
message(STATUS "  ODB source path: ${SERVER_ROOT}/src/Database")
message(STATUS "  PROTO_GEN_DIR: ${PROTO_GEN_DIR}")
message(STATUS "=========================================")

# ===== ПОИСК ИСХОДНИКОВ СЕРВЕРА =====
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/**/*.hpp")
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")

# Исключаем dev файлы
list(FILTER HEADERS EXCLUDE REGEX ".*_dev\\.hpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*_dev\\.cpp$")

# Исключаем ODB модели из основного списка
list(FILTER HEADERS EXCLUDE REGEX ".*/Database/.*\\.hxx$")
list(FILTER SOURCES EXCLUDE REGEX ".*-odb\\.(cpp|cxx)$")

set(SERVER_SOURCES
    ${HEADERS}
    ${SOURCES}
)

# ===== НАСТРОЙКА PKG_CONFIG =====
set(ENV{PKG_CONFIG_PATH} "/usr/lib/x86_64-linux-gnu/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "")

find_package(PkgConfig REQUIRED)

pkg_check_modules(PGSQL REQUIRED libpq)
pkg_check_modules(ODB REQUIRED odb libodb-pgsql)
pkg_check_modules(GRPC REQUIRED grpc++ grpc gpr)
pkg_check_modules(Sodium REQUIRED libsodium)
pkg_check_modules(UUID REQUIRED uuid)

# ===== ДОБАВЛЯЕМ PROTO ЗАВИСИМОСТЬ =====
if(TARGET proto_generated)
    message(STATUS "✅ proto_generated target found")
    set(HAVE_PROTO TRUE)
else()
    message(WARNING "⚠️ proto_generated target not found, gRPC may not work")
    set(HAVE_PROTO FALSE)
endif()

# ===== ПОИСК ODB ФАЙЛОВ =====
set(ODB_DIR "${SERVER_ROOT}/src/Database")
message(STATUS "Looking for ODB files in: ${ODB_DIR}")

if(NOT EXISTS ${ODB_DIR})
    message(FATAL_ERROR "Database directory not found: ${ODB_DIR}")
endif()

# Ищем .hxx файлы
file(GLOB_RECURSE ODB_FILES "${ODB_DIR}/*.hxx")
list(FILTER ODB_FILES EXCLUDE REGEX ".*-odb\\.(hxx|cxx|ixx)$")

# Дополнительно ищем .hpp файлы с ODB аннотациями
file(GLOB_RECURSE ODB_HPP_FILES "${ODB_DIR}/*.hpp")
foreach(FILE ${ODB_HPP_FILES})
    file(STRINGS ${FILE} CONTENT LIMIT_COUNT 1 REGEX "#pragma db object")
    if(CONTENT)
        list(APPEND ODB_FILES ${FILE})
    endif()
endforeach()

list(REMOVE_DUPLICATES ODB_FILES)

if(NOT ODB_FILES)
    message(FATAL_ERROR "No ODB files found in ${ODB_DIR}")
endif()

message(STATUS "Found ODB files:")
foreach(ODB_FILE ${ODB_FILES})
    message(STATUS "  - ${ODB_FILE}")
endforeach()

# ===== ГЕНЕРАЦИЯ ODB КОДА =====
file(MAKE_DIRECTORY ${ODB_GEN_DIR})
set(ODB_GENERATED_SRCS "")

foreach(ODB_FILE ${ODB_FILES})
    get_filename_component(ODB_NAME ${ODB_FILE} NAME_WE)
    
    set(ODB_HDR "${ODB_GEN_DIR}/${ODB_NAME}-odb.hxx")
    set(ODB_SRC "${ODB_GEN_DIR}/${ODB_NAME}-odb.cxx")
    
    message(STATUS "Generating ODB for: ${ODB_NAME}")
    
    add_custom_command(
        OUTPUT ${ODB_HDR} ${ODB_SRC}
        COMMAND odb -d pgsql 
                --std c++20
                --generate-query 
                --generate-session
                -I${SERVER_ROOT}
                --output-dir ${ODB_GEN_DIR}
                ${ODB_FILE}
        DEPENDS ${ODB_FILE}
        COMMENT "Generating ODB code for ${ODB_NAME}"
        VERBATIM
    )
    
    list(APPEND ODB_GENERATED_SRCS ${ODB_SRC})
endforeach()

# Если нет файлов для генерации, создаём пустой список
if(NOT ODB_GENERATED_SRCS)
    message(FATAL_ERROR "No ODB sources generated")
endif()

# ===== БИБЛИОТЕКА ODB =====
add_library(odb_generated STATIC ${ODB_GENERATED_SRCS})

set_source_files_properties(${ODB_GENERATED_SRCS} PROPERTIES
    LANGUAGE CXX
    GENERATED TRUE
)

target_link_libraries(odb_generated
    ${PGSQL_LIBRARIES}
    ${ODB_LIBRARIES}
)

target_include_directories(odb_generated PUBLIC
    ${ODB_GEN_DIR}
    ${SERVER_ROOT}
    ${SERVER_ROOT}/src/Database
    ${PGSQL_INCLUDE_DIRS}
    ${ODB_INCLUDE_DIRS}
)

# ===== ПАРАМЕТРЫ ПОДКЛЮЧЕНИЯ К БД =====
set(DB_HOST "localhost" CACHE STRING "PostgreSQL host")
set(DB_PORT "5432" CACHE STRING "PostgreSQL port")
set(DB_NAME "desk_db" CACHE STRING "PostgreSQL database name")
set(DB_USER "desk_admin" CACHE STRING "PostgreSQL user")
set(DB_PASSWORD "desk_admin" CACHE STRING "PostgreSQL password")

set(CREATE_TABLE_SQL "${ODB_GEN_DIR}/Models.sql")

# ===== ГЕНЕРАЦИЯ SQL СХЕМЫ =====
find_program(ODB_COMPILER odb REQUIRED)

# Генерируем SQL из всех ODB файлов
add_custom_command(
    OUTPUT ${CREATE_TABLE_SQL}
    COMMAND ${ODB_COMPILER} 
        -d pgsql              
        --std c++20
        --generate-schema-only
        --schema-format sql
        --at-once
        ${ODB_FILES}
    DEPENDS ${ODB_FILES}
    COMMENT "Generating database schema SQL from ODB models..."
    VERBATIM
)

add_custom_target(generate_schema DEPENDS ${CREATE_TABLE_SQL})

# ===== СОЗДАНИЕ SH СКРИПТА =====
set(DB_SETUP_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/setup_db.sh")

# Создаём скрипт настройки БД
file(WRITE ${DB_SETUP_SCRIPT} 
    "#!/bin/bash\n"
    "# Database auto-setup script\n\n"
    "set -e\n\n"
    "export PGPASSWORD=\"${DB_PASSWORD}\"\n\n"
    "echo \"==========================================\"\n"
    "echo \"Setting up database: ${DB_NAME}\"\n"
    "echo \"==========================================\"\n\n"
    "# Create database if not exists\n"
    "if ! psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -lqt | cut -d \\| -f 1 | grep -qw \"${DB_NAME}\"; then\n"
    "    echo \"Creating database '${DB_NAME}'...\"\n"
    "    createdb -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} ${DB_NAME}\n"
    "    echo \"✅ Database created\"\n"
    "else\n"
    "    echo \"✅ Database '${DB_NAME}' already exists\"\n"
    "fi\n\n"
    "# Apply schema\n"
    "echo \"Applying schema...\"\n"
    "psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -f ${CREATE_TABLE_SQL}\n"
    "echo \"✅ Schema applied\"\n\n"
    "# Show tables\n"
    "echo \"\"\n"
    "echo \"Current tables:\"\n"
    "psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -c \"\\dt\"\n"
    "echo \"\"\n"
    "echo \"==========================================\"\n"
    "echo \"✅ Database setup complete!\"\n"
    "echo \"==========================================\"\n"
)

# Делаем скрипт исполняемым
file(CHMOD ${DB_SETUP_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# Цель для настройки БД

# ===== УДАЛЯЕМ execute_process (он выполняется при конфигурации) =====
# Вместо execute_process используем custom_target

# ===== ОСНОВНОЙ ИСПОЛНЯЕМЫЙ ФАЙЛ =====
# Проверяем, есть ли исходники
if(NOT SERVER_SOURCES)
    message(WARNING "No server sources found, creating minimal main.cpp")
    set(SERVER_MAIN "${SERVER_ROOT}/src/main.cpp")
    file(WRITE ${SERVER_MAIN} 
        "#include <iostream>\n"
        "#include <odb/pgsql/database.hxx>\n\n"
        "int main() {\n"
        "    std::cout << \"Server starting...\\n\";\n"
        "    return 0;\n"
        "}\n")
    set(SERVER_SOURCES ${SERVER_MAIN})
endif()

message(STATUS "Server sources count: ${SERVER_SOURCES}")

add_executable(desk_server ${SERVER_SOURCES})

# Базовые библиотеки
set(DESK_SERVER_LIBRARIES
    odb_generated
    ${PGSQL_LIBRARIES}
    ${ODB_LIBRARIES}
    ${Sodium_LIBRARIES}
    ${UUID_LIBRARIES}
    pthread
    dl
    rt
)

if(HAVE_PROTO)
    list(APPEND DESK_SERVER_LIBRARIES
        proto_generated
        ${GRPC_LIBRARIES}
        ${Protobuf_LIBRARIES}
    )
endif()

target_link_libraries(desk_server PRIVATE ${DESK_SERVER_LIBRARIES})

# Include директории
set(DESK_SERVER_INCLUDES
    ${SERVER_ROOT}
    ${SERVER_ROOT}/src
    ${SERVER_ROOT}/src/Database
    ${ODB_GEN_DIR}
    ${PGSQL_INCLUDE_DIRS}
    ${ODB_INCLUDE_DIRS}
    ${Sodium_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
)

if(HAVE_PROTO)
    list(APPEND DESK_SERVER_INCLUDES
        ${PROTO_GEN_DIR}
        ${GRPC_INCLUDE_DIRS}
        ${Protobuf_INCLUDE_DIRS}
    )
endif()

target_include_directories(desk_server PRIVATE ${DESK_SERVER_INCLUDES})

target_compile_options(desk_server PRIVATE
    ${Sodium_CFLAGS_OTHER}
    -Wall
    -Wextra
    -std=c++20 
)

target_compile_definitions(desk_server PRIVATE
    GRPC_USE_REFLECTION
)

# ===== ЗАВИСИМОСТИ =====
add_dependencies(desk_server odb_generated generate_schema)

if(HAVE_PROTO)
    add_dependencies(desk_server proto_generated)
endif()

# ===== ФИНАЛЬНАЯ ИНФОРМАЦИЯ =====
message(STATUS "=========================================")
message(STATUS "Configuration Summary:")
message(STATUS "  SERVER_ROOT: ${SERVER_ROOT}")
message(STATUS "  ODB files found: ${ODB_FILES}")
message(STATUS "  ODB generated dir: ${ODB_GEN_DIR}")
message(STATUS "  Proto support: ${HAVE_PROTO}")
message(STATUS "  Database: ${DB_NAME} on ${DB_HOST}:${DB_PORT} as ${DB_USER}")
message(STATUS "=========================================")
message(STATUS "Database setup targets:")
message(STATUS "  make setup_database  - Full setup (create DB + schema)")
message(STATUS "=========================================")
message(STATUS "Usage:")
message(STATUS "  1. First time: make setup_database")
message(STATUS "  2. Normal build: make desk_server")
message(STATUS "  3. After model changes: make setup_database && make desk_server")
message(STATUS "=========================================")
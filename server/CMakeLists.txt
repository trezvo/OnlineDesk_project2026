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

# ===== ГЕНЕРАЦИЯ ODB КОДА (без --schema-file) =====
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
                --generate-query 
                --generate-schema
                -I${SERVER_ROOT}
                --output-dir ${ODB_GEN_DIR}
                ${ODB_FILE}
        DEPENDS ${ODB_FILE}
        COMMENT "Generating ODB code for ${ODB_NAME}"
        VERBATIM
    )
    
    list(APPEND ODB_GENERATED_SRCS ${ODB_SRC} ${ODB_HDR})
endforeach()

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

# ===== СОЗДАНИЕ ОБЪЕДИНЕННОЙ СХЕМЫ =====
set(COMBINED_SCHEMA "${ODB_GEN_DIR}/combined_schema.sql")

# Команда для объединения всех .sql файлов
add_custom_command(
    OUTPUT ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "-- Combined ODB Schema" > ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "-- Generated automatically from ODB models" >> ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "" >> ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "DROP SCHEMA public CASCADE;" >> ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "CREATE SCHEMA public;" >> ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "" >> ${COMBINED_SCHEMA}
    COMMAND for f in ${ODB_GEN_DIR}/*.sql; do echo "-- $$(basename $$f)"; cat $$f; echo ""; done >> ${COMBINED_SCHEMA}
    DEPENDS ${ODB_GENERATED_SRCS}
    COMMENT "Combining ODB schema files"
    VERBATIM
)

# Альтернативный способ без цикла в shell
add_custom_command(
    OUTPUT ${COMBINED_SCHEMA}.tmp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Combined ODB Schema" > ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "-- Generated automatically from ODB models" >> ${COMBINED_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E echo "" >> ${COMBINED_SCHEMA}
    DEPENDS ${ODB_GENERATED_SRCS}
)

# ===== ЦЕЛЬ ДЛЯ АВТОМАТИЧЕСКОГО ПРИМЕНЕНИЯ СХЕМЫ =====
# Создаем скрипт для полной настройки БД
set(DB_SETUP_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/setup_database.sh")

file(WRITE ${DB_SETUP_SCRIPT} 
    "#!/bin/bash\n"
    "# Auto-setup script for ODB database\n\n"
    "set -e\n\n"
    "echo \"==========================================\"\n"
    "echo \"Setting up database for ODB\"\n"
    "echo \"==========================================\"\n\n"
    "export PGPASSWORD=\"${DB_PASSWORD}\"\n\n"
    "# 1. Проверка/создание базы данных\n"
    "echo \"1. Checking database...\"\n"
    "if psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -lqt | cut -d \\| -f 1 | grep -qw \"${DB_NAME}\"; then\n"
    "    echo \"   Database ${DB_NAME} already exists\"\n"
    "else\n"
    "    echo \"   Creating database ${DB_NAME}...\"\n"
    "    createdb -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} ${DB_NAME}\n"
    "    echo \"   ✅ Database created\"\n"
    "fi\n\n"
    "# 2. Генерация схемы из ODB\n"
    "echo \"2. Generating schema from ODB models...\"\n"
    "cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
    "for ODB_FILE in ${ODB_FILES}; do\n"
    "    echo \"   Processing: $(basename $ODB_FILE)\"\n"
    "    odb -d pgsql --generate-schema \"$ODB_FILE\" > \"${ODB_GEN_DIR}/$(basename ${ODB_FILE} .hxx).sql\"\n"
    "done\n\n"
    "# 3. Объединение всех схем\n"
    "echo \"3. Combining schema files...\"\n"
    "cat ${ODB_GEN_DIR}/*.sql > ${ODB_GEN_DIR}/full_schema.sql\n\n"
    "# 4. Применение схемы\n"
    "echo \"4. Applying schema to database...\"\n"
    "psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -f ${ODB_GEN_DIR}/full_schema.sql\n\n"
    "echo \"\"\n"
    "echo \"==========================================\"\n"
    "echo \"✅ Database setup complete!\"\n"
    "echo \"==========================================\"\n"
)

file(CHMOD ${DB_SETUP_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# ===== ПАРАМЕТРЫ ПОДКЛЮЧЕНИЯ =====
set(DB_HOST "localhost" CACHE STRING "PostgreSQL host")
set(DB_PORT "5432" CACHE STRING "PostgreSQL port")
set(DB_NAME "desk_db" CACHE STRING "PostgreSQL database name")
set(DB_USER "desk_admin" CACHE STRING "PostgreSQL user")
set(DB_PASSWORD "desk_admin" CACHE STRING "PostgreSQL password")

# ===== СОЗДАНИЕ SQL СКРИПТА ДЛЯ ТАБЛИЦЫ =====
set(CREATE_TABLE_SQL "${CMAKE_CURRENT_BINARY_DIR}/create_table.sql")

file(WRITE ${CREATE_TABLE_SQL} 
    "CREATE TABLE IF NOT EXISTS \"User\" (\n"
    "    \"id\" BIGSERIAL NOT NULL PRIMARY KEY,\n"
    "    \"uuid\" TEXT NOT NULL UNIQUE,\n"
    "    \"username\" TEXT NOT NULL UNIQUE,\n"
    "    \"password\" TEXT NOT NULL\n"
    ");\n"
    "\n"
    "CREATE INDEX IF NOT EXISTS \"idx_User_username\" ON \"User\" (\"username\");\n"
    "CREATE INDEX IF NOT EXISTS \"idx_User_uuid\" ON \"User\" (\"uuid\");\n"
)

# ===== СОЗДАНИЕ SH СКРИПТА =====
set(DB_SETUP_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/setup_db.sh")

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
    "# Create table\n"
    "echo \"Creating table 'User'...\"\n"
    "psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -f ${CREATE_TABLE_SQL}\n"
    "echo \"✅ Table created\"\n\n"
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

# ===== ВАРИАНТ 1: Выполнение во время конфигурации =====
execute_process(
    COMMAND bash ${DB_SETUP_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE setup_result
    OUTPUT_VARIABLE setup_output
    ERROR_VARIABLE setup_error
)

if(setup_result EQUAL 0)
    message(STATUS "✅ Database setup successful")
    message(STATUS "${setup_output}")
else()
    message(WARNING "Database setup failed: ${setup_error}")
    message(STATUS "You can run manually: bash ${DB_SETUP_SCRIPT}")
endif()


# Цель для настройки БД
add_custom_target(setup_database
    COMMAND ${DB_SETUP_SCRIPT}
    COMMENT "Setting up database (create + schema)"
)

# Цель для применения схемы (без пересоздания БД)
set(APPLY_SCHEMA_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/apply_schema.sh")

file(WRITE ${APPLY_SCHEMA_SCRIPT} 
    "#!/bin/bash\n"
    "# Apply only schema (database must exist)\n\n"
    "set -e\n\n"
    "export PGPASSWORD=\"${DB_PASSWORD}\"\n\n"
    "echo \"Generating and applying schema...\"\n"
    "cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
    "> ${ODB_GEN_DIR}/combined.sql\n"
    "for ODB_FILE in ${ODB_FILES}; do\n"
    "    echo \"   Processing: $(basename $ODB_FILE)\"\n"
    "    odb -d pgsql --at-once --generate-schema \"$ODB_FILE\" >> ${ODB_GEN_DIR}/combined.sql\n"
    "done\n"
    "echo \"Applying schema...\"\n"
    "psql -h ${DB_HOST} -p ${DB_PORT} -U ${DB_USER} -d ${DB_NAME} -f ${ODB_GEN_DIR}/combined.sql\n"
    "echo \"✅ Schema applied\"\n"
)

file(CHMOD ${APPLY_SCHEMA_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

add_custom_target(apply_schema
    COMMAND ${APPLY_SCHEMA_SCRIPT}
    DEPENDS ${ODB_GENERATED_SRCS}
    COMMENT "Applying ODB schema to database"
)

# ===== ОСНОВНОЙ ИСПОЛНЯЕМЫЙ ФАЙЛ =====
if(NOT SERVER_SOURCES)
    message(WARNING "No server sources found, creating minimal main.cpp")
    file(WRITE "${SERVER_ROOT}/src/main.cpp" 
        "#include <iostream>\nint main() {\n    std::cout << \"Server starting...\\n\";\n    return 0;\n}\n")
    set(SERVER_SOURCES "${SERVER_ROOT}/src/main.cpp")
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
)

target_compile_definitions(desk_server PRIVATE
    GRPC_USE_REFLECTION
)

# ===== ЗАВИСИМОСТИ =====
add_dependencies(desk_server odb_generated)

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
message(STATUS "  make apply_schema    - Apply schema only (DB must exist)")
message(STATUS "=========================================")
message(STATUS "Usage:")
message(STATUS "  1. First time: make setup_database")
message(STATUS "  2. Normal build: make desk_server")
message(STATUS "  3. After model changes: make apply_schema && make desk_server")
message(STATUS "=========================================")
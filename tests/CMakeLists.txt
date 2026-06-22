cmake_minimum_required(VERSION 3.20)
project(unit_tests
        VERSION 0.0.1
        DESCRIPTION "Unit tests for TgBot App"
        LANGUAGES CXX)

set(INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/domain/entities
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/domain/interfaces
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/infrastructure/sqlite3
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        )

set(TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/units/domain/entities/user_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/units/domain/entities/user_repository_sqlite_test.cpp
        )
set(APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/infrastructure/sqlite3/user_repository_sqlite.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/infrastructure/sqlite3/watch_link_repository_sqlite.cpp
        )
add_executable(${PROJECT_NAME}
                ${TEST_SOURCES}
                ${APP_SOURCES}
                )

target_include_directories(${PROJECT_NAME} PRIVATE
${INCLUDES}
${CMAKE_CURRENT_SOURCE_DIR})

find_package(SQLite3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE curlpp SQLite::SQLite3)

find_package(GTest REQUIRED)
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
                        GTest::gtest_main      
                        GTest::gmock_main
                        pthread
                        )
target_compile_definitions(${PROJECT_NAME} PRIVATE 
                        TEST_DB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
                    )
                    
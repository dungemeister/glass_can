cmake_minimum_required(VERSION 3.20)

project(glass_can
        VERSION 0.0.1
        DESCRIPTION "Telegram Bot for steam traders to check steam orders"
        LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SOURCES main.cpp
            src/bot.cpp
            src/bot_context.cpp
            src/db.cpp
            src/worker_pool.cpp
            )

set(INCLUDES third_party
             src
             )

add_library(curlpp SHARED IMPORTED)
set_target_properties(curlpp PROPERTIES
                      IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/third_party/curlpp/libcurlpp.so.1.0.0"
                      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/curlpp/"
                      INTERFACE_LINK_LIBRARIES "ssl;crypto;curl" 
)
find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME}  PROPERTIES
    BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/curlpp"
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/curlpp"
)
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE curlpp SQLite::SQLite3)

cmake_minimum_required(VERSION 3.20)

project(glass_can
        VERSION 0.0.1
        DESCRIPTION "Telegram Bot for steam traders to check steam orders"
        LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


enable_testing()

add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

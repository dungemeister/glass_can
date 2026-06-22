

set(SOURCES main.cpp
            # bot.cpp
            # db/db.cpp
            # core/bot_context.cpp
            # core/periodic_pool.cpp
            core/worker_pool.cpp
            infrastructure/sqlite3/user_repository_sqlite.cpp
            infrastructure/sqlite3/watch_link_repository_sqlite.cpp
            infrastructure/telegram/telegram_client.cpp
            )

set(INCLUDES third_party
             core
             core/tools
             db
             domain/entities
             domain/events
             domain/telegram
             domain/interfaces
             application/use_cases
             infrastructure
             infrastructure/sqlite3
             infrastructure/telegram
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


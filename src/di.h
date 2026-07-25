#pragma once
#include "user_service.h"
#include "telegram_client.h"
#include "bot_config.h"

#include "user_repository_sqlite.h"
#include "watch_link_repository_sqlite.h"
#include "purchase_repository_sqlite.h"

#include "worker_pool.h"
#include "event_handler.h"
#include "log_macros.h"
#include <thread>

struct CompositionContainer{
    std::shared_ptr<UserSqliteRepository> m_user_rep;
    std::shared_ptr<WatchLinkSqliteRepository> m_watch_rep;
    std::shared_ptr<PurchaseRepositorySqlite> m_purchase_rep;

    std::shared_ptr<UserService> m_user_service;
    std::shared_ptr<TelegramClient> m_telegram_client;
    std::shared_ptr<Sqlite3Connection> m_db_connection;
    std::shared_ptr<EventHandler> m_event_handler;
    std::shared_ptr<WorkerPool> m_workers_pool;

    CompositionContainer()
    {
        std::filesystem::path currentDir;
        try {
            currentDir = std::filesystem::current_path();

        } catch (std::filesystem::filesystem_error const& ex) {
            std::cout << "Error: " << ex.what() << std::endl;
            exit(-1);
        }
        std::string config_file = currentDir.string() + "/config.json";
        Sqlite3Config db_config("db/steam.db");

        m_db_connection = std::make_shared<Sqlite3Connection>(db_config);

        m_user_rep = std::make_shared<UserSqliteRepository>(m_db_connection);
        m_watch_rep = std::make_shared<WatchLinkSqliteRepository>(m_db_connection);
        m_purchase_rep = std::make_shared<PurchaseRepositorySqlite>(m_db_connection);
        m_user_service = std::make_shared<UserService>(*m_user_rep);

        BotConfig bot_config(config_file);
        m_telegram_client = std::make_shared<TelegramClient>(bot_config.getBotToken());
        m_workers_pool = std::make_shared<WorkerPool>(bot_config.getBotWorkers());

        m_event_handler = std::make_shared<EventHandler>(m_telegram_client,
                                                        m_user_rep,
                                                        m_watch_rep,
                                                        m_purchase_rep);

    }

    void run(){
        int64_t offset = 0;
        while(true){
            auto events = m_telegram_client->getUpdates(offset);
            LOG_DEBUG("GOT UPDATES");
            
            for(const auto& event: events){
                // offset = update["update_id"].get<uint64_t>() + 1;
                m_workers_pool->enqueue([this, event]{
                    EventHandler::ResponseCallback cb = [](const std::string& msg){
                        LOG_INFO(msg);
                    };
                    m_event_handler->execute(event, cb);
                    // handleUpdate(update);
                    // std::cout << msg.get_chat().get_username().value() << ": " << msg.get_text().value() << std::endl;
                });

            }
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};
#pragma once
#include "sqlite_config.h"
#include <sqlite3.h>
#include <stdexcept>
#include <mutex>

class Sqlite3Connection{
public:
    Sqlite3Connection(const Sqlite3Config& config)
    :m_database_path(config.database_path)
    {
        auto exit = sqlite3_open(m_database_path.c_str(), &m_db);
        if(exit != SQLITE_OK){
            throw std::runtime_error("Database " + m_database_path + " Error: " + sqlite3_errmsg(m_db));
        }
        if(config.wal_mode){
            execute("PRAGMA journal_mode=WAL;");
            execute("PRAGMA synchronous=NORMAL;");
        }
    }

    Sqlite3Connection(const Sqlite3Connection& ) = delete;
    Sqlite3Connection& operator=(const Sqlite3Connection&) = delete;
    
    sqlite3* getHandler() const { return m_db; }

    void beginTransaction(){
        execute("BEGIN DEFERRED;");
    }
    void commit(){
        execute("COMMIT;");
    }
    void rollback(){
        execute("ROLLBACK;");
    }
    
    void execute(const std::string& sql){
        std::unique_lock lock(m_mutex);
        char* err_msg = nullptr;
        auto rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);
        if(rc != SQLITE_OK){
            DATABASE_THROW_EXCEPTION(err_msg);
        }
    }
private:
    sqlite3* m_db;
    std::string m_database_path;
    std::mutex m_mutex;
};

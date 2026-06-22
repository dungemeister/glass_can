#pragma once
#include <memory>
#include <string>

struct Sqlite3Config{
    Sqlite3Config(const std::string& file)
    :database_path(file)
    ,wal_mode(true) {}
    
    std::string database_path;
    bool wal_mode;

};
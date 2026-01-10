#pragma once
#include <string>
#include <sqlite3.h>
#include <vector>
#include "nlohmann/json.hpp"

class DataBase{
public:
    DataBase(const std::string& file);
    ~DataBase();

    void exec(const std::string& sql);
    std::vector<nlohmann::json> query(const std::string& sql);
    
    void initSchema();

    int64_t addUser(int64_t chat_id, const std::string& username, const std::string& first_name);
    int64_t getUserId(int64_t chat_id);

    std::vector<nlohmann::json> getUserLinks(uint64_t chat_id);
    bool addUserLink(uint64_t chat_id, const std::string& link, const std::string& title);
    bool deleteUserLink(uint64_t chat_id, const std::string& title);

private:
    std::string m_file;
    sqlite3* m_db;
};
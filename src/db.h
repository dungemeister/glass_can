#pragma once
#include <string>
#include <sqlite3.h>
#include <vector>
#include "nlohmann/json.hpp"

#include "user_context.h"
#include "string_misc.h"

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
    nlohmann::json getUserLinkByTitle(uint64_t chat_id, const std::string& title);
    bool addUserLink(uint64_t chat_id, const std::string& link, const std::string& title);
    bool deleteUserLink(uint64_t chat_id, const std::string& title);

    bool addUserItemBuyInfo(uint64_t chat_id, const UserContext::ItemBuyInfo& info);
    nlohmann::json deleteUserItemBuyInfo(uint64_t chat_id, const std::string& title);
    nlohmann::json getUserItemsBuyInfo(uint64_t chat_id);

    nlohmann::json setUserCurrency(uint64_t chat_id, const std::string& currency);
private:
    std::string m_file;
    sqlite3* m_db;
};
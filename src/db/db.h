#pragma once
#include <string>
#include <sqlite3.h>
#include <vector>
#include "nlohmann/json.hpp"

#include "user_context.h"
#include "string_misc.h"
#include "survey_link.h"
#include "db_pagination.h"

class DataBase{
public:
    
    DataBase(const std::string& file);
    ~DataBase();

    void exec(const std::string& sql);
    std::vector<nlohmann::json> query(const std::string& sql);
    
    void initSchema();

    int64_t addUser(int64_t chat_id, const std::string& username, const std::string& first_name);
    int64_t getUserId(int64_t chat_id);
    //Watch list functions
    std::vector<nlohmann::json> getUserLinks(uint64_t chat_id);
    nlohmann::json getUserLinkByTitle(uint64_t chat_id, const std::string& title);
    bool addUserLink(uint64_t chat_id, const std::string& link, const std::string& title);
    bool deleteUserLink(uint64_t chat_id, const std::string& title);
    //Purchaised list functions
    bool addUserItemBuyInfo(uint64_t chat_id, const UserContext::ItemBuyInfo& info);
    nlohmann::json deleteUserItemBuyInfo(uint64_t chat_id, const std::string& title, float buy_price, int amount);
    nlohmann::json getUserItemsBuyInfo(uint64_t chat_id);
    //Survey list functions
    nlohmann::json addUserSurveyLink(uint64_t chat_id, const std::string& title, const std::string link, int period);
    nlohmann::json getUserSurveyLinks(uint64_t chat_id, const DataBasePagination& pag);
    //Other functions
    nlohmann::json setUserCurrency(uint64_t chat_id, const std::string& currency);
private:
    std::string m_file;
    sqlite3* m_db;
};
#pragma once
#include <string>
#include <sqlite3.h>
#include <vector>
#include "nlohmann/json.hpp"

#include "user_context.h"
#include "string_misc.h"
#include "survey_link.h"
#include "db_pagination.h"
#include "watch_link.h"
#include "purchased_item.h"
#include "user.h"


class DataBase{
public:
    enum UserId{
        UNKNOWN_USER = -1,
    };
    DataBase(const std::string& file);
    ~DataBase();

    void exec(const std::string& sql);
    std::vector<nlohmann::json> query(const std::string& sql);
    
    void initSchema();

    int64_t addUser(int64_t chat_id, const std::string& username, const std::string& first_name);
    std::optional<int64_t> getUserId(int64_t chat_id);
    std::vector<User> getUsers(const DataBasePagination& pag);
    //Watch list functions
    std::vector<WatchLink> getUserLinks(uint64_t chat_id);
    std::vector<WatchLink> getUserLinksJoin(uint64_t chat_id);
    std::optional<WatchLink> getUserLinkByTitle(uint64_t chat_id, const std::string& title);
    bool addUserLink(uint64_t chat_id, const std::string& link, const std::string& title);
    bool deleteUserLink(uint64_t chat_id, const std::string& title);
    //Purchaised list functions
    bool addUserItemBuyInfo(uint64_t chat_id, const UserContext::ItemBuyInfo& info);
    std::optional<PurchasedItem> deleteUserItemBuyInfo(uint64_t chat_id, const std::string& title, float buy_price, int amount);
    std::vector<PurchasedItem> getUserItemsBuyInfo(uint64_t chat_id);
    std::vector<PurchasedItem> getUserItemsBuyInfoJoin(uint64_t chat_id);
    //Survey list functions
    int64_t                     addUserSurveyLink(const SurveyLink& survey_link);
    std::vector<SurveyLink>     getUserSurveyLinks(uint64_t chat_id, const DataBasePagination& pag);
    std::vector<SurveyLink>     deleteSurveyLink(uint64_t chat_id, const std::string& title);
    //Other functions
    nlohmann::json setUserCurrency(uint64_t chat_id, const std::string& currency);
private:
    std::string m_file;
    sqlite3* m_db;

    void debugPrintQuery(sqlite3_stmt* stmt);
};
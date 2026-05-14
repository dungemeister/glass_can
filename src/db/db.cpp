#include "db.h"
#include <iostream>
#include <sstream>
#include "survey_link.h"

DataBase::DataBase(const std::string& file)
:m_file(file)
{
    auto exit = sqlite3_open(file.c_str(), &m_db);
    if(exit){
        std::cerr << "Database " << file << sqlite3_errmsg(m_db) << std::endl;
    }
    else{
        initSchema();
    }
}

DataBase::~DataBase(){
    sqlite3_close(m_db);
}

void DataBase::initSchema(){
    std::string q = R"(
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        chat_id INTEGER UNIQUE NOT NULL,
        username TEXT,
        first_name TEXT,
        currency TEXT DEFAULT USD,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS user_links (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL,
        url TEXT NOT NULL UNIQUE,
        title TEXT,
        last_checked DATETIME,
        status TEXT DEFAULT 'active',
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
    );

    CREATE TABLE IF NOT EXISTS items_buy_info (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL,
        url TEXT NOT NULL,
        title TEXT,
        buy_price FLOAT NOT NULL,
        amount INTEGER NOT NULL,
        date TEXT -- YYYY-MM-DD
    );

    CREATE TABLE IF NOT EXISTS user_survey_tasks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER REFERENCES users(id),
        url TEXT NOT NULL,
        title TEXT,
        period INTEGER NOT NULL,
        date TEXT -- YYYY-MM-DD,
        task_id_hash BLOB NOT NULL
    ); )";

    sqlite3_stmt* stmt;
    try{
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        debugPrintQuery(stmt);
        if(sqlite3_step(stmt) != SQLITE_DONE){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to execute stmt: " + sqlite3_errmsg(m_db));
        }
        
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt);
}

int64_t DataBase::addUser(int64_t chat_id, const std::string& username = "", 
                      const std::string& first_name = ""){
    int64_t user_id = -1;
    std::string q = R"(
    INSERT OR IGNORE INTO users (chat_id, username, first_name)
    VALUES (?,?,?)
    RETURNING id;)";
    sqlite3_stmt* stmt;
    try{
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_int(stmt, 1, chat_id);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, first_name.c_str(), -1, SQLITE_TRANSIENT);

        debugPrintQuery(stmt);

        while(sqlite3_step(stmt) == SQLITE_ROW){
            user_id = sqlite3_column_int64(stmt, 0);
        }
        if(sqlite3_step(stmt) == SQLITE_DONE){
            sqlite3_finalize(stmt);
        }
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return user_id;
}

std::optional<int64_t> DataBase::getUserId(int64_t chat_id){
    int64_t user_id = UserId::UNKNOWN_USER;
    std::string q = R"(
    SELECT users.id
    FROM users
    WHERE chat_id = ?;)";
    sqlite3_stmt* stmt;
    try{
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_int64(stmt, 1, chat_id);
        debugPrintQuery(stmt);
        if(sqlite3_step(stmt) == SQLITE_ROW){
            user_id = sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
        }
        else{
            sqlite3_finalize(stmt);
            return std::nullopt;
        }
    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return user_id;
}

std::vector<WatchLink> DataBase::getUserLinks(uint64_t chat_id){
    std::vector<WatchLink> res;

    std::string q = R"(
    SELECT users.chat_id, links.id, links.user_id, links.url, links.title, links.status
    FROM user_links AS links
    JOIN users ON users.id == links.user_id
    WHERE users.chat_id == ?;
    )";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
    }

    sqlite3_bind_int64(stmt, 1, chat_id);
    debugPrintQuery(stmt);

    while(sqlite3_step(stmt) == SQLITE_ROW){
        WatchLink link;
        link.chat_id = sqlite3_column_int64(stmt, 0);
        link.url = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        link.title = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        link.enabled = true;

        res.emplace_back(std::move(link));
    }
    return res;
}

std::vector<WatchLink> DataBase::getUserLinksJoin(uint64_t chat_id){
    std::vector<WatchLink> res;
    res.reserve(10);

    std::string q = R"(
    SELECT users.chat_id, user_links.url, user_links.title, user_links.status
    FROM users
    JOIN user_links ON user_links.user_id == users.id
    WHERE users.chat_id == ?;
    )";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    debugPrintQuery(stmt);

    while(sqlite3_step(stmt) == SQLITE_ROW){
        WatchLink link;
        link.chat_id    = sqlite3_column_int64(stmt, 0);
        link.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        link.title      = std::string (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        link.enabled    = true;
        res.emplace_back(std::move(link));
    }
    sqlite3_finalize(stmt);
    return std::move(res);
}

std::optional<WatchLink> DataBase::getUserLinkByTitle(uint64_t chat_id, const std::string& title){
    std::string q = R"(
    SELECT users.chat_id, user_links.url, user_links.title, user_links.status
    FROM users
    INNER JOIN user_links ON users.id == user_links.user_id
    WHERE users.chat_id = ? AND user_links.title = ?;
    )";
    sqlite3_stmt* stmt;
    try{
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_int64(stmt, 1, chat_id);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);

        if(sqlite3_step(stmt) == SQLITE_ROW){
            WatchLink link;
            link.chat_id    = sqlite3_column_int64(stmt, 0);
            link.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            link.title      = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            link.enabled    = true;
            sqlite3_finalize(stmt);
            return link;
        }
    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool DataBase::addUserLink(uint64_t chat_id, const std::string& link, const std::string& title){
    try{
        auto user_id_opt = getUserId(chat_id);
        if(!user_id_opt.has_value()) return {};
    
        auto user_id = user_id_opt.value();
        if(user_id == UserId::UNKNOWN_USER) return {};
        std::stringstream q;
        q << "INSERT INTO user_links (user_id, url, title) " <<
             "VALUES (" << user_id << ", '" << link << "', '" << title << "');";
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "addUserLink: " << e.what() << std::endl;
        return false;
    }
}

bool DataBase::deleteUserLink(uint64_t chat_id, const std::string& title){
    try{
            auto user_id_opt = getUserId(chat_id);
    if(!user_id_opt.has_value()) return {};
    
    auto user_id = user_id_opt.value();
    if(user_id == UserId::UNKNOWN_USER) return {};
        std::stringstream q;
        q << "DELETE FROM user_links WHERE user_id = " << user_id <<
             " AND title = '" << title << "';";
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "deleteUserLink: " << e.what() << std::endl;
        return false;
    }
}

bool DataBase::addUserItemBuyInfo(uint64_t chat_id, const UserContext::ItemBuyInfo& item){
    int64_t item_id = -1;
    try{
            auto user_id_opt = getUserId(chat_id);
    if(!user_id_opt.has_value()) return {};
    
    auto user_id = user_id_opt.value();
    if(user_id == UserId::UNKNOWN_USER) return {};
        auto link_opt = getUserLinkByTitle(chat_id, item.title);
        if(!link_opt.has_value()) return false;
        auto& link = link_opt.value();
        std::string q = R"(
        INSERT INTO items_buy_info (user_id, url, title, buy_price, amount, date)
        VALUES (?,?,?,?,?,?)
        RETURNING id;
        )";
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));

        }

        sqlite3_bind_int   (stmt, 1, user_id);
        sqlite3_bind_text  (stmt, 2, link.url.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt, 3, item.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, static_cast<double>(item.buy_price));
        sqlite3_bind_int   (stmt, 5, item.amount);
        sqlite3_bind_text  (stmt, 6, item.buy_date.c_str(), -1, SQLITE_TRANSIENT);

        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            item_id = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);

    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return item_id;
}

std::optional<PurchasedItem> DataBase::deleteUserItemBuyInfo(uint64_t chat_id, const std::string& title, float buy_price, int amount){
    std::string q1 = R"(
    SELECT users.id
    FROM users
    WHERE chat_id = ?
    )";
    std::string q2 = R"(
    DELETE FROM items_buy_info
    WHERE title = ? AND user_id = ? AND buy_price = ? AND amount = ?
    RETURNING user_id, url, title, buy_price, amount, date;
    )";
    sqlite3_stmt* stmt1;
    sqlite3_stmt* stmt2;
    int64_t user_id = 0;
    try{
        if(sqlite3_prepare_v2(m_db, q1.c_str(), -1, &stmt1, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_int64(stmt1, 1, chat_id);
        debugPrintQuery(stmt1);

        if(sqlite3_step(stmt1) == SQLITE_ROW){
            user_id = sqlite3_column_int64(stmt1, 0);
        }
        else{
            DATABASE_THROW_EXCEPTION("SQLITE3 fail: " + sqlite3_errmsg(m_db));
        }
        if(user_id <= 0){
            DATABASE_THROW_EXCEPTION("NOT FOUND User with chat_id " + std::to_string(chat_id));
        }
        if(sqlite3_prepare_v2(m_db, q2.c_str(), -1, &stmt2, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }

        sqlite3_bind_text(stmt2, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt2, 2, user_id);
        sqlite3_bind_double(stmt2, 3, static_cast<double>(buy_price));
        sqlite3_bind_int64(stmt2, 4, amount);
        debugPrintQuery(stmt2);

        if(sqlite3_step(stmt2) == SQLITE_ROW){
            PurchasedItem item;
            item.user_id    = sqlite3_column_int64(stmt2, 0);
            item.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 1)));
            item.title      = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 2)));
            item.buy_price  = static_cast<float>(sqlite3_column_double(stmt2, 3));
            item.amount     = sqlite3_column_int64(stmt2, 4);
            item.date       = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 5)));

            sqlite3_finalize(stmt2);
            return item;
        }
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt1);
    sqlite3_finalize(stmt2);
    return std::nullopt;
}

std::vector<PurchasedItem> DataBase::getUserItemsBuyInfo(uint64_t chat_id){
    std::vector<PurchasedItem> res;
    try{
        auto user_id_opt = getUserId(chat_id);
        if(!user_id_opt.has_value()) return {};
        
        auto user_id = user_id_opt.value();
        if(user_id == UserId::UNKNOWN_USER) return {};
        std::string q = R"(
        SELECT * FROM items_buy_info WHERE user_id = ?;
        )";
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }

        sqlite3_bind_int64(stmt, 1, user_id);
        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            PurchasedItem item;
            item.user_id    = sqlite3_column_int64(stmt, 1);
            item.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            item.title      = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            item.buy_price  = static_cast<float>(sqlite3_column_double(stmt, 4));
            item.amount     = sqlite3_column_int64(stmt, 5);
            item.date       = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));

            res.emplace_back(std::move(item));
        }
        sqlite3_finalize(stmt);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return res;
}

std::vector<PurchasedItem> DataBase::getUserItemsBuyInfoJoin(uint64_t chat_id){
    std::vector<PurchasedItem> res;
    try{
        std::string q = R"(
        SELECT items.user_id, items.url, items.title, items.buy_price, items.amount, items.date
        FROM items_buy_info AS items
        JOIN users ON items.user_id == users.id
        WHERE users.chat_id == ?;
        )";
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }

        sqlite3_bind_int64(stmt, 1, chat_id);
        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            PurchasedItem item;
            item.user_id    = sqlite3_column_int64(stmt, 0);
            item.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            item.title      = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            item.buy_price  = static_cast<float>(sqlite3_column_double(stmt, 3));
            item.amount     = sqlite3_column_int64(stmt, 4);
            item.date       = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));

            res.emplace_back(std::move(item));
        }
        sqlite3_finalize(stmt);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return res;
}

nlohmann::json DataBase::setUserCurrency(uint64_t chat_id, const std::string& currency){
    nlohmann::json result;
    try{
        std::stringstream q;
        q << "INSERT INTO users\n" <<
             "SET currency = '" << currency <<"'\n" <<
             "WHERE chat_id = " << chat_id << ";";
        result["data"] = nlohmann::json::array();
        result["error_msg"] = "";
        result["ok"] = true;

    }
    catch(const std::exception& e){
        result["data"] = nlohmann::json::array();
        result["error_msg"] = std::string(e.what());
        result["ok"] = false;
    }
    return result;
}

int64_t DataBase::addUserSurveyLink(const SurveyLink& survey_link){
    int64_t link_id = -1;
    try{
        
        std::string q = R"(
        INSERT INTO user_survey_tasks (user_id, url, title, period, date, task_id_hash)
        VALUES (?,?,?,?,?,?);
        )";
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }

        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(survey_link.chat_id));
        sqlite3_bind_text (stmt, 2, survey_link.url.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 3, survey_link.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int  (stmt, 4, survey_link.period);
        sqlite3_bind_text (stmt, 5, "2026-03-03", -1, SQLITE_STATIC);
        sqlite3_bind_blob (stmt, 6, &survey_link.task_id_hash, sizeof(survey_link.task_id_hash), SQLITE_TRANSIENT);

        if (auto code = sqlite3_step(stmt); code != SQLITE_DONE) {
            // обработка ошибки
            sqlite3_finalize(stmt);
            throw std::runtime_error("ERROR: SQLITE3 fail " + std::string(sqlite3_errmsg(m_db)));
        }
        debugPrintQuery(stmt);

        link_id = sqlite3_last_insert_rowid(m_db);
        sqlite3_finalize(stmt);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return link_id;
}

std::vector<SurveyLink> DataBase::getUserSurveyLinks(uint64_t chat_id, const DataBasePagination& pag){
    std::vector<SurveyLink> links;
    try{
        std::string q = R"(
        SELECT * FROM user_survey_tasks
        WHERE user_id = ? ORDER BY id;
        )";

        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_int(stmt, 1, chat_id);
        debugPrintQuery(stmt);

        while(sqlite3_step(stmt) == SQLITE_ROW){
            SurveyLink link;
            link.chat_id = sqlite3_column_int(stmt, 1);
            link.url    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            link.title  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            link.period = sqlite3_column_int(stmt, 4);
            auto blob = sqlite3_column_blob(stmt, 6);
            if(blob && sqlite3_column_bytes(stmt, 6) == sizeof(size_t)){
                memcpy(&link.task_id_hash, blob, sizeof(link.task_id_hash));
            }
            links.emplace_back(std::move(link));
        }

        sqlite3_finalize(stmt);

    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return links;
}

std::vector<SurveyLink> DataBase::deleteSurveyLink(uint64_t chat_id, const std::string& title){
    std::vector<SurveyLink> res;
    res.reserve(10);
    try{
        std::string q = R"(
        DELETE FROM user_survey_tasks WHERE title = ? RETURNING url, title, period, task_id_hash;
        )";

        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);

        while(sqlite3_step(stmt) == SQLITE_ROW){
            SurveyLink link;
            link.chat_id = chat_id;
            link.url = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            link.title = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            link.period = sqlite3_column_int64(stmt, 2);
            auto blob = sqlite3_column_blob(stmt, 3);
            if(blob && sqlite3_column_bytes(stmt, 0) == sizeof(link.task_id_hash)){
                memcpy(&link.task_id_hash, blob, sizeof(link.task_id_hash));
            }
            res.push_back(link);
        }
        sqlite3_finalize(stmt);

    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return res;
}

std::vector<User> DataBase::getUsers(const DataBasePagination& pag){
    std::vector<User> res;
    sqlite3_stmt* stmt;
    std::string q=R"(
    SELECT * FROM users
    )";
    if(pag.limit > 0 || pag.offset > 0){
        q += " LIMIT " + std::to_string(pag.limit) + " OFFSET " + std::to_string(pag.offset) + ";";
    }
    else{
        q += ";";
    }

    try{
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(m_db));
        }

        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            User user;
            user.chat_id    = sqlite3_column_int64(stmt, 1);
            user.username   = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            user.first_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            user.created_at = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            user.currency   = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            
            res.emplace_back(std::move(user));
        }
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt);
    return res;
}

void DataBase::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            std::cout << "Expanded SQL: " << expanded_sql << std::endl;
            sqlite3_free(expanded_sql);
        }
    #endif
}
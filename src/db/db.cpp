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
    try{
        exec(R"(
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
                task_id_hash BLOB NOT NULL DEFAULT X''
            );

        )");
    }
    catch(const std::exception& e){
        std::cerr << "initSchema: " << e.what() << std::endl;
    }
}

void DataBase::exec(const std::string& sql){
    // (INSERT/UPDATE/DELETE)
    std::cout << "EXEC: " << sql << std::endl;
    char* error = nullptr;
    if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        std::string err = error;
        sqlite3_free(error);
        throw std::runtime_error("SQL error: " + err);
    }
}

int64_t DataBase::addUser(int64_t chat_id, const std::string& username = "", 
                      const std::string& first_name = ""){
    try{

        std::stringstream command;
        command << "INSERT OR IGNORE INTO users (chat_id, username, first_name)\n VALUES (" <<
                chat_id << ", '" <<
                username << "', '" <<
                first_name << "');";
        query(command.str());
        return getUserId(chat_id);
    }
    catch(const std::exception& e){
        std::cerr << "addUser: " << e.what() << std::endl;
        return -1;
    }
}

int64_t DataBase::getUserId(int64_t chat_id){
    std::stringstream q;
    q << "SELECT id FROM users WHERE chat_id = " <<
         chat_id << ";";
    auto users = query(q.str());
    if(users.empty()){
        throw std::runtime_error("getUserId: user with chat_id=" + std::to_string(chat_id) + " not found");
    }
    auto data = users[0]["id"].get<std::string>();
    return std::atol(data.c_str());
}

std::vector<nlohmann::json> DataBase::query(const std::string& sql){
    std::cout << "QUERY: " << sql << std::endl;

    std::vector<nlohmann::json> rows;
    char* err = nullptr;

    auto cb = [](void* data, int argc, char** argv, char** col) -> int {
        auto* out = static_cast<std::vector<nlohmann::json>*>(data);

        nlohmann::json row;
        for (int i = 0; i < argc; ++i) {
            // argv[i] == nullptr means SQL NULL
            row[col[i]] = argv[i] ? argv[i] : "";
        }
        out->push_back(std::move(row));
        return 0; // 0 = continue
    };
    if(auto res = sqlite3_exec(m_db, sql.c_str(), cb, &rows, &err);res != SQLITE_OK){
        std::string error_msg = err ? err: "Unknown Error";
        sqlite3_free(err);
        throw std::runtime_error("SQL query error: " + error_msg);
    }

    return rows;
}

std::vector<nlohmann::json> DataBase::getUserLinks(uint64_t chat_id){
    auto user_id = getUserId(chat_id);
    std::stringstream q;
    q << "SELECT * FROM user_links WHERE user_id = " << user_id << " ORDER BY id DESC;";
    auto stats = query(q.str());
    return stats;
}

nlohmann::json DataBase::getUserLinkByTitle(uint64_t chat_id, const std::string& title){
    auto user_id = getUserId(chat_id);
    std::stringstream q;
    q << "SELECT * FROM user_links WHERE user_id = " << user_id << " AND title = '" << title << "';";
    auto stats = query(q.str());
    if(stats.empty()) return {};
    return stats[0];
}

bool DataBase::addUserLink(uint64_t chat_id, const std::string& link, const std::string& title){
    try{
        auto user_id = getUserId(chat_id);
        std::stringstream q;
        q << "INSERT INTO user_links (user_id, url, title) " <<
             "VALUES (" << user_id << ", '" << link << "', '" << title << "');";
        exec(q.str());
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "addUserLink: " << e.what() << std::endl;
        return false;
    }
}

bool DataBase::deleteUserLink(uint64_t chat_id, const std::string& title){
    try{
        auto user_id = getUserId(chat_id);
        std::stringstream q;
        q << "DELETE FROM user_links WHERE user_id = " << user_id <<
             " AND title = '" << title << "';";
        exec(q.str());
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "deleteUserLink: " << e.what() << std::endl;
        return false;
    }
}

bool DataBase::addUserItemBuyInfo(uint64_t chat_id, const UserContext::ItemBuyInfo& info){
    try{
        // id INTEGER PRIMARY KEY AUTOINCREMENT,
        // user_id INTEGER NOT NULL,
        // url TEXT NOT NULL UNIQUE,
        // title TEXT,
        // buy_price FLOAT NOT NULL,
        // amount INTEGER NOT NULL,
        // date TEXT -- YYYY-MM-DD

        auto user_id = getUserId(chat_id);
        auto row = getUserLinkByTitle(chat_id, info.title);

        std::stringstream q;
        q << "INSERT INTO items_buy_info (user_id, url, title, buy_price, amount, date) " <<
             "VALUES (" << user_id << ", '" << StringMisc::sqlQuoteShielding(StringMisc::removeQuotes(row["url"])) << "', '" << info.title << "', '" <<
                           info.buy_price << "', '" << info.amount << "', '" << info.buy_date << "');";
        exec(q.str());
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "addUserItemButInfo: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::json DataBase::deleteUserItemBuyInfo(uint64_t chat_id, const std::string& title, float buy_price, int amount){
    nlohmann::json result;
    try{
        auto user_id = getUserId(chat_id);
        std::stringstream q;
        q  << std::fixed << std::setprecision(2) <<
             "DELETE FROM items_buy_info WHERE title = '" << title << "'" <<
             "AND user_id = " << user_id <<
             " AND buy_price = " << buy_price <<
             " AND amount = " << amount << ";";
             
        exec(q.str());
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

nlohmann::json DataBase::getUserItemsBuyInfo(uint64_t chat_id){
    nlohmann::json result;
    try{
        auto user_id = getUserId(chat_id);
        std::stringstream q;
        q << "SELECT * FROM items_buy_info WHERE user_id = " << user_id << ";";
        result["data"] = query(q.str());
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

nlohmann::json DataBase::setUserCurrency(uint64_t chat_id, const std::string& currency){
    nlohmann::json result;
    try{
        std::stringstream q;
        q << "INSERT INTO users\n" <<
             "SET currency = '" << currency <<"'\n" <<
             "WHERE chat_id = " << chat_id << ";";
        exec(q.str());
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
            throw std::runtime_error("ERROR: SQLITE3 fail to prepare stmt");
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
            throw std::runtime_error("ERROR: SQLITE3 fail " + std::string(sqlite3_errmsg(m_db)));
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

std::vector<size_t> DataBase::deleteSurveyLink(uint64_t chat_id, const std::string& title){
    std::vector<size_t> hashes;
    hashes.reserve(10);
    try{
        std::string q = R"(
        DELETE FROM user_survey_tasks WHERE title = ? RETURNING task_id_hash;
        )";

        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            throw std::runtime_error("ERROR: SQLITE3 fail " + std::string(sqlite3_errmsg(m_db)));
        }
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);

        while(sqlite3_step(stmt) == SQLITE_ROW){
            size_t hash;
            auto blob = sqlite3_column_blob(stmt, 0);
            if(blob && sqlite3_column_bytes(stmt, 0) == sizeof(hash)){
                memcpy(&hash, blob, sizeof(hash));
            }
            hashes.push_back(hash);
            std::cout << "Deleted task with id " << hash << std::endl;
        }
        sqlite3_finalize(stmt);

    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return hashes;
}

nlohmann::json DataBase::getUsers(const DataBasePagination& pag){
    nlohmann::json result;

    try{
        std::stringstream q;
        q << "SELECT * FROM users";
        if(pag.limit != 0){
            q << " LIMIT " << pag.limit << " " <<
                 "OFFSET " << pag.offset;
        }
        q << ";";
        auto rows = query(q.str());

        result["data"] = rows;
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

void DataBase::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            std::cout << "Expanded SQL: " << expanded_sql << std::endl;
            sqlite3_free(expanded_sql);
        }
    #endif
}
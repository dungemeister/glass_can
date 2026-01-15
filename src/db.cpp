#include "db.h"
#include <iostream>
#include <sstream>


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

            CREATE TABLE items_buy_info (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                url TEXT NOT NULL,
                title TEXT,
                buy_price FLOAT NOT NULL,
                amount INTEGER NOT NULL,
                date TEXT -- YYYY-MM-DD
            );

        )");
    }
    catch(const std::exception& e){
        std::cerr << "initSchema: " << e.what() << std::endl;
    }
}

void DataBase::exec(const std::string& sql){
    // (INSERT/UPDATE/DELETE)
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
    std::vector<nlohmann::json> rows;
    char* err = nullptr;

    auto cb = [](void* data, int argc, char** argv, char** col) -> int {
        auto* out = static_cast<std::vector<nlohmann::json>*>(data);

        nlohmann::json row;
        for (int i = 0; i < argc; ++i) {
            // argv[i] == nullptr означает SQL NULL
            row[col[i]] = argv[i] ? argv[i] : "";
        }
        out->push_back(std::move(row));
        return 0; // 0 = продолжать
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
             "VALUES (" << user_id << ", '" << StringMisc::removeQuotes(row["url"]) << "', '" << info.title << "', '" <<
                           info.buy_price << "', '" << info.amount << "', '" << info.buy_date << "');";
        exec(q.str());
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "addUserItemButInfo: " << e.what() << std::endl;
        return false;
    }
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
        result["error_msg"] = e.what();
        result["ok"] = false;
    }
    return result;
}
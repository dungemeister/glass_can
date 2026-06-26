#include "user_repository_sqlite.h"
#include <iostream>

std::vector<User> UserSqliteRepository::getUsers(const DataBasePagination& pag){
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
        auto db = m_conn->getHandler();
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
        }

        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            User user;
            user.set_id(sqlite3_column_int64(stmt, 0));
            user.set_chat_id(sqlite3_column_int64(stmt, 1));
            user.set_username(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
            user.set_first_name(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
            user.set_created_at(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))));
            user.set_currency(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))));
            
            res.emplace_back(std::move(user));
        }
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt);
    return res;
}

std::optional<User> UserSqliteRepository::getUser(int chat_id){
    std::string q = R"(
    SELECT * FROM users
    WHERE chat_id = ?;)";
    sqlite3_stmt* stmt;
    try{
        auto db = m_conn->getHandler();
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
        }
        sqlite3_bind_int64(stmt, 1, chat_id);
        debugPrintQuery(stmt);
        if(sqlite3_step(stmt) == SQLITE_ROW){
            User user;
            user.set_id(sqlite3_column_int64(stmt, 0));
            user.set_chat_id(sqlite3_column_int64(stmt, 1));
            user.set_username(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
            user.set_first_name(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
            user.set_created_at(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))));
            user.set_currency(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5))));
            user.set_state(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))));
            
            sqlite3_finalize(stmt);
            return user;
        }
    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

void UserSqliteRepository::save(const User& user){
    std::string q = R"(
    INSERT OR REPLACE INTO users (id, chat_id, username, first_name, created_at, currency, state) 
    VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* stmt;
    try{
        auto db = m_conn->getHandler();
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
        }
        if(user.get_id() > 0){
            sqlite3_bind_int64(stmt, 1, user.get_id());
        }
        sqlite3_bind_int64(stmt, 2, user.get_chat_id());
        sqlite3_bind_text(stmt, 3, user.get_username().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, user.get_first_name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, user.get_created_at().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, user.get_currency().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, user.get_state().c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);
        if(sqlite3_step(stmt) == SQLITE_DONE){
            
        }
        sqlite3_finalize(stmt);
    }
    catch(std::exception& e){
        std::cerr << e.what() << std::endl;
    }
}

void UserSqliteRepository::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            std::cout << "Expanded SQL: " << expanded_sql << std::endl;
            sqlite3_free(expanded_sql);
        }
    #endif
}
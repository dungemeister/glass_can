#include "purchase_repository_sqlite.h"
#include <iostream>

std::vector<PurchasedItem> PurchaseRepositorySqlite::getItems(const User& user){
    std::vector<PurchasedItem> res;
    try{

    std::string q = R"(
        SELECT * FROM items_buy_info WHERE user_id = ?;
        )";
        sqlite3_stmt* stmt;
        auto db = m_conn->getHandler();
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
        }

        sqlite3_bind_int64(stmt, 1, user.get_id());
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

void PurchaseRepositorySqlite::addItem(const User& user, const PurchasedItem& item){
    std::string q = R"(
        INSERT INTO items_buy_info (user_id, url, title, buy_price, amount, date)
        VALUES (?,?,?,?,?,?)
        RETURNING id;
        )";
    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    int64_t item_id = -1;
    try{
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));

        }

        sqlite3_bind_int   (stmt, 1, user.get_id());
        sqlite3_bind_text  (stmt, 2, item.url.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt, 3, item.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, static_cast<double>(item.buy_price));
        sqlite3_bind_int   (stmt, 5, item.amount);
        sqlite3_bind_text  (stmt, 6, item.date.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            item_id = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
}

void PurchaseRepositorySqlite::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            std::cout << "Expanded SQL: " << expanded_sql << std::endl;
            sqlite3_free(expanded_sql);
        }
    #endif
}

void PurchaseRepositorySqlite::removeItem(const User& user, const PurchasedItem& item){
    std::string q = R"(
        DELETE from items_buy_info
        WHERE user_id=? AND url=? AND title=? AND buy_price=? AND amount=? AND date=?;)";

    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    int64_t item_id = -1;
    try{
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));

        }

        sqlite3_bind_int   (stmt, 1, user.get_id());
        sqlite3_bind_text  (stmt, 2, item.url.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt, 3, item.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, static_cast<double>(item.buy_price));
        sqlite3_bind_int   (stmt, 5, item.amount);
        sqlite3_bind_text  (stmt, 6, item.date.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            item_id = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
}

PurchasedItem PurchaseRepositorySqlite::getItemByTitle(const User& user, const std::string& title){
    std::string q = R"(
        SELECT * from items_buy_info
        WHERE user_id=? AND title=?;)";

    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    int64_t item_id = -1;
    try{
        if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
            DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));

        }

        sqlite3_bind_int   (stmt, 1, user.get_id());
        sqlite3_bind_text  (stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
        debugPrintQuery(stmt);
        if(sqlite3_step(stmt) == SQLITE_ROW){
            PurchasedItem item;
            item.user_id    = sqlite3_column_int64(stmt, 1);
            item.url        = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            item.title      = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            item.buy_price  = static_cast<float>(sqlite3_column_double(stmt, 4));
            item.amount     = sqlite3_column_int64(stmt, 5);
            item.date       = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            
            sqlite3_finalize(stmt);
            return item;
        }
        sqlite3_finalize(stmt);
        
        
    }
    catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
    }
    return {};
}
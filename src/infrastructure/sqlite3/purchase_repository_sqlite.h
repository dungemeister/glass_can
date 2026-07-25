#pragma once
#include "sqlite_connection.h"
#include "purchase_irepository.h"
#include <memory>

#define DATABASE_DEBUG

class PurchaseRepositorySqlite: public IPurchaseRepository{
public:
    PurchaseRepositorySqlite(const std::shared_ptr<Sqlite3Connection>& conn)
    :m_conn(conn)
    {}
    ~PurchaseRepositorySqlite() override {}
    std::vector<PurchasedItem> getItems(const User& user);
    
    void addItem(const User& user, const PurchasedItem& item);
    void removeItem(const User& user, const PurchasedItem& item);
    PurchasedItem getItemByTitle(const User& user, const std::string& title);
private:
    std::shared_ptr<Sqlite3Connection> m_conn;
    void debugPrintQuery(sqlite3_stmt* stmt);
};
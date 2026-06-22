#pragma once
#include "watch_link_irepository.h"
#include "sqlite_connection.h"
#include <memory>

class WatchLinkSqliteRepository: public IWatchLinkRepository{
public:
    WatchLinkSqliteRepository(const std::shared_ptr<Sqlite3Connection>& conn)
    :m_conn(conn) {}
    ~WatchLinkSqliteRepository() override {}

    std::vector<WatchLink>  getLinks(const User& user);
    void                    addLink(const WatchLink& link);
    void                    deleteLink(const WatchLink& link);
private:
    std::shared_ptr<Sqlite3Connection> m_conn;

    void debugPrintQuery(sqlite3_stmt* stmt);
};

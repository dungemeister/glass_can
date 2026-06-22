#include "watch_link_repository_sqlite.h"
#include "sqlite3.h"
#include "misc.h"



std::vector<WatchLink> WatchLinkSqliteRepository::getLinks(const User& user){
    std::vector<WatchLink> res;
    auto chat_id = user.get_chat_id();
    std::string q = R"(
    SELECT users.chat_id, links.id, links.user_id, links.url, links.title, links.status
    FROM user_links AS links
    JOIN users ON users.id == links.user_id
    WHERE users.chat_id == ?;
    )";
    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
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

void WatchLinkSqliteRepository::addLink(const WatchLink& link){

}
void WatchLinkSqliteRepository::deleteLink(const WatchLink& link){

}

void WatchLinkSqliteRepository::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            std::cout << "Expanded SQL: " << expanded_sql << std::endl;
            sqlite3_free(expanded_sql);
        }
    #endif
}
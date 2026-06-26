#include "watch_link_repository_sqlite.h"
#include "sqlite3.h"
#include "misc.h"
#include "log_macros.h"

#define DATABASE_DEBUG

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
    sqlite3_finalize(stmt);
    return res;
}

void WatchLinkSqliteRepository::addLink(const WatchLink& link){

    std::string q = R"(
    INSERT INTO user_links (user_id, url, title)
    SELECT id, ?, ?
    FROM users
    WHERE chat_id=?;
    )";

    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));

    }

    sqlite3_bind_text  (stmt, 1, link.url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 2, link.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (stmt, 3, link.chat_id);
    debugPrintQuery(stmt);

    if(sqlite3_step(stmt) != SQLITE_DONE){
        sqlite3_finalize(stmt);
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to execute stmt: " + sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);

}

void WatchLinkSqliteRepository::deleteLink(const WatchLink& link){
    std::string q = R"(
    DELETE FROM user_links
    WHERE title = ?
    AND user_id = (SELECT id FROM users WHERE chat_id = ?)
    )";

    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));

    }

    sqlite3_bind_text  (stmt, 1, link.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (stmt, 2, link.chat_id);
    debugPrintQuery(stmt);

    if(sqlite3_step(stmt) != SQLITE_DONE){
        sqlite3_finalize(stmt);
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to execute stmt: " + sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

WatchLink WatchLinkSqliteRepository::getLinkFromTitle(const User& user, const std::string& title){
    WatchLink link;
    auto chat_id = user.get_chat_id();
    std::string q = R"(
    SELECT users.chat_id, links.id, links.user_id, links.url, links.title, links.status
    FROM user_links AS links
    JOIN users ON users.id = links.user_id
    WHERE users.chat_id = ? AND links.title = ?;
    )";
    sqlite3_stmt* stmt;
    auto db = m_conn->getHandler();
    if(sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        DATABASE_THROW_EXCEPTION("SQLITE3 fail to prepare stmt: " + sqlite3_errmsg(db));
    }

    sqlite3_bind_int64(stmt, 1, chat_id);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    debugPrintQuery(stmt);

    if(auto res = sqlite3_step(stmt); res == SQLITE_ROW ){
        // sqlite3_finalize(stmt);
        link.chat_id = sqlite3_column_int64(stmt, 0);
        link.url = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        link.title = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        link.enabled = true;
        sqlite3_finalize(stmt);
    }
    else{
        DATABASE_THROW_EXCEPTION("Title \"" + title + "\" not found");
    }
    
    return link;
}

void WatchLinkSqliteRepository::debugPrintQuery(sqlite3_stmt* stmt) {
    #ifdef DATABASE_DEBUG
        char* expanded_sql = sqlite3_expanded_sql(stmt);
        if (expanded_sql) {
            LOG_INFO(expanded_sql);
            sqlite3_free(expanded_sql);
        }
    #endif
}
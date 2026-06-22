#pragma once
#include "user_irepository.h"
#include "sqlite_connection.h"
#include "misc.h"
#include "db_pagination.h"

#define DATABASE_DEBUG

class UserSqliteRepository: public IUserRepository {
public:

    UserSqliteRepository(const std::shared_ptr<Sqlite3Connection>& conn)
    :m_conn(conn)
    {}
    ~UserSqliteRepository() override {}
    
    std::vector<User>   getUsers(const DataBasePagination& pag);
    std::optional<User> getUser(int chat_id);

    void                save(const User& user);
private:
    std::shared_ptr<Sqlite3Connection> m_conn;

    void debugPrintQuery(sqlite3_stmt* stmt);
};
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "user.h"
#include "user_repository_sqlite.h"
#include <memory>

class SqliteUserRepositoryFixture: public ::testing::Test{
public:
    const int expected_chat_id = 444431591;
    const std::string old_first_name = "Юрий";
    const std::string new_first_name = "Новое Тестовое Имя";
    std::shared_ptr<UserSqliteRepository> rep;
    void SetUp(){
        std::string db_dir = TEST_DB_DIR;
        Sqlite3Config config(db_dir + "/db/steam.db");
        std::shared_ptr<Sqlite3Connection> conn = std::make_shared<Sqlite3Connection>(config);
        rep = std::make_shared<UserSqliteRepository>(conn);
    }
    void TearDown(){

    }
};

TEST_F(SqliteUserRepositoryFixture, AddUser){
    auto new_user_chat_id = -1;
    User user(-1, new_user_chat_id, "test_username", "test_name", "29-05-2026", "USD");
    rep->save(user);

    auto added_user_opt = rep->getUser(new_user_chat_id);
    EXPECT_EQ(added_user_opt.has_value(), true);

    auto added_user = added_user_opt.value();
    EXPECT_EQ(user.get_username(), added_user.get_username());
    EXPECT_EQ(user.get_first_name(), added_user.get_first_name());
    EXPECT_EQ(user.get_chat_id(), added_user.get_chat_id());
    EXPECT_EQ(user.get_created_at(), added_user.get_created_at());
    EXPECT_EQ(user.get_currency(), added_user.get_currency());

}

TEST_F(SqliteUserRepositoryFixture, RemoveUser){

}

TEST_F(SqliteUserRepositoryFixture, ChangeFirstName){
    auto user_opt = rep->getUser(expected_chat_id);
    EXPECT_EQ(user_opt.has_value(), true);

    auto user = user_opt.value();
    user.set_first_name(new_first_name);
    rep->save(user);
    
    auto updated_user_opt  = rep->getUser(expected_chat_id);
    user = updated_user_opt.value();
    EXPECT_EQ(user.get_first_name(), new_first_name);

}

TEST_F(SqliteUserRepositoryFixture, RestoreFirstName){
    auto user_opt = rep->getUser(expected_chat_id);
    EXPECT_EQ(user_opt.has_value(), true);

    auto user = user_opt.value();
    user.set_first_name(old_first_name);
    rep->save(user);
    
    auto updated_user_opt  = rep->getUser(expected_chat_id);
    user = updated_user_opt.value();
    EXPECT_EQ(user.get_first_name(), old_first_name);

}
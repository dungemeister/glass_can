#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "user.h"

struct UserTestClass: public ::testing::Test{
    User* m_user;

    void SetUp() { m_user = new User(1, 1, "TestUser", "TestFirstName", "03-03-2026", "usd");}
    void TearDown() {delete m_user;}

};


TEST_F(UserTestClass, ConstructorTest){
    // Arange
    
    // Act
    auto id         = m_user->get_id();
    auto chat_id    = m_user->get_chat_id();
    auto username   = m_user->get_username();
    auto first_name = m_user->get_first_name();
    auto created_at = m_user->get_created_at();
    auto currency   = m_user->get_currency();
    
    // Assert
    ASSERT_EQ(id, 1);
    ASSERT_EQ(chat_id, 1);
    ASSERT_EQ(username, "TestUser");
    ASSERT_EQ(first_name, "TestFirstName");
    ASSERT_EQ(created_at, "03-03-2026");
    ASSERT_EQ(currency, "usd");
}

TEST_F(UserTestClass, SetterGetterTest){
    auto new_chat_id = 4444;
    auto new_username = "New User Name";
    auto new_first_name = "New First Name";
    auto new_created_at = "01-01-1970";
    auto new_currency = "RUB";

    m_user->set_chat_id(new_chat_id);
    m_user->set_username(new_username);
    m_user->set_first_name(new_first_name);
    m_user->set_created_at(new_created_at);
    m_user->set_currency(new_currency);

    EXPECT_EQ(m_user->get_chat_id(), new_chat_id);
    EXPECT_EQ(m_user->get_username(), new_username);
    EXPECT_EQ(m_user->get_first_name(), new_first_name);
    EXPECT_EQ(m_user->get_created_at(), new_created_at);
    EXPECT_EQ(m_user->get_currency(), new_currency);
}

TEST_F(UserTestClass, ActivateDeactivateTest){
    m_user->deactivate();
    EXPECT_EQ(m_user->is_active(), false);

    m_user->activate();
    EXPECT_EQ(m_user->is_active(), true);

}
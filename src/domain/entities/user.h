#pragma once
#include <string>
#include "misc.h"


struct User{
public:
    using UserState = std::string;

    User(   int64_t _id,   
            int64_t _chat_id,
            const std::string& _username,
            const std::string& _first_name,
            const std::string& _created_at,
            const std::string& _currency,
            const UserState&   _state
     )
    :m_id(_id)
    ,m_chat_id(_chat_id)
    ,m_username(_username)
    ,m_first_name(_first_name)
    ,m_created_at(_created_at)
    ,m_currency(_currency)
    ,m_active(true)
    ,m_state(_state)
    {}

    User() :User(-1, -1, "", "", "", "", ""){}

    bool operator==(const User& other) const{
        return (m_chat_id == other.m_chat_id &&
                m_username == other.m_username &&
                m_first_name == other.m_first_name &&
                m_created_at == other.m_created_at &&
                m_currency == other.m_currency);
    }

    int64_t     get_id() const          { return m_id; }
    int64_t     get_chat_id() const     { return m_chat_id; }
    std::string get_username() const    { return m_username; }
    std::string get_first_name() const  { return m_first_name; }
    std::string get_created_at() const  { return m_created_at; }
    std::string get_currency() const    { return m_currency; }
    std::string get_state() const       { return m_state; }
    bool        is_active() const       { return m_active; }

    void        set_id(int64_t _id)                             { m_id = _id; }
    void        set_chat_id(int64_t _chat_id)                   { m_chat_id = _chat_id; }
    void        set_username(const std::string& _username)      { m_username = _username; }
    void        set_first_name(const std::string& _first_name)  { m_first_name = _first_name; }
    void        set_created_at(const std::string& _created_at)  { m_created_at = _created_at; }
    void        set_currency(const std::string& _currency)      { m_currency = _currency; }
    void        set_state(const std::string& _state)            { m_state = _state; }
    
    void        deactivate()    { m_active = false; }
    void        activate()      { m_active = true; }
private:        
    int64_t     m_id;
    int64_t     m_chat_id;
    std::string m_username;
    std::string m_first_name;
    std::string m_created_at;
    std::string m_currency;
    UserState   m_state;

    bool m_active;

    std::string repr() const { return "Username (" + std::to_string(m_chat_id) + "):" + m_username + "\n"
                                      "First name:" + m_first_name + "\n"
                                      "Currency: " + m_currency + "\n"
                                      "State: " + m_state; }
};
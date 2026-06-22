#pragma once
#include <string>
#include "misc.h"

struct User{
public:
    User(   int64_t _id,   
            int64_t _chat_id,
            const std::string& _username,
            const std::string& _first_name,
            const std::string& _created_at,
            const std::string& _currency
     )
    :id(_id)
    ,chat_id(_chat_id)
    ,username(_username)
    ,first_name(_first_name)
    ,created_at(_created_at)
    ,currency(_currency)
    ,active(true)
    {}

    User() :User(-1, -1, "", "", "", ""){}

    bool operator==(const User& other) const{
        return (chat_id == other.chat_id &&
                username == other.username &&
                first_name == other.first_name &&
                created_at == other.created_at &&
                currency == other.currency);
    }

    int64_t     get_id() const          { return id; }
    int64_t     get_chat_id() const     { return chat_id; }
    std::string get_username() const    { return username; }
    std::string get_first_name() const  { return first_name; }
    std::string get_created_at() const  { return created_at; }
    std::string get_currency() const    { return currency; }
    bool        is_active() const       { return active; }

    void        set_id(int64_t _id) { id = _id; }
    void        set_chat_id(int64_t _chat_id) { chat_id = _chat_id; }
    void        set_username(const std::string& _username) { username = _username; }
    void        set_first_name(const std::string& _first_name) { first_name = _first_name; }
    void        set_created_at(const std::string& _created_at) { created_at = _created_at; }
    void        set_currency(const std::string& _currency) { currency = _currency; }

    void        deactivate()    { active = false; }
    void        activate()      { active = true; }
private:        
    int64_t id;
    int64_t chat_id;
    std::string username;
    std::string first_name;
    std::string created_at;
    std::string currency;

    bool active;

    std::string repr() const { return std::to_string(chat_id) + ": " + username + " " + first_name + " currency: " + currency; }
};
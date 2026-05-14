#pragma once
#include <string>

struct User{
    int64_t chat_id;
    std::string username;
    std::string first_name;
    std::string created_at;
    std::string currency;

    std::string repr() const { return std::to_string(chat_id) + ": " + username + " " + first_name + " currency: " + currency; }
};
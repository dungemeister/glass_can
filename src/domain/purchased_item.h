#pragma once
#include <string>

struct PurchasedItem{
    int64_t     user_id;
    std::string url;
    std::string title;
    float       buy_price;
    int64_t     amount;
    std::string date;
};
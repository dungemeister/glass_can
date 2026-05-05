#pragma once
#include <string>
#include <stdint.h>

namespace UserContext{
    struct ItemBuyInfo{
        uint64_t    chat_id;
        std::string user_link;
        std::string title;
        float       buy_price;
        uint64_t    amount;
        std::string buy_date;
    };
}; //namespace UserContext
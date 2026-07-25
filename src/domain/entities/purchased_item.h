#pragma once
#include <string>

struct PurchasedItem{
    PurchasedItem(  int64_t             user_id_,
                    const std::string&  url_,
                    const std::string&  title_,
                    float               buy_price_,
                    int64_t             amount_,
                    const std::string&  date_
     )
     :user_id(user_id_)
     ,url(url_)
     ,title(title_)
     ,buy_price(buy_price_)
     ,amount(amount_)
     ,date(date_)
     {}

    PurchasedItem()
    :PurchasedItem(0, "", "", 0.0f, 0, "")
    {}

    uint64_t        user_id;
    std::string     url;
    std::string     title;
    float           buy_price;
    int64_t         amount;
    std::string     date;
};
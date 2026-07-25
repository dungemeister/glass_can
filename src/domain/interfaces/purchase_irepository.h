#pragma once
#include "user.h"
#include "purchased_item.h"
#include <vector>

class IPurchaseRepository{
public:    
    virtual ~IPurchaseRepository() = default;

    virtual std::vector<PurchasedItem> getItems(const User& user) = 0;
    virtual void addItem(const User& user, const PurchasedItem& item) = 0;
    virtual void removeItem(const User& user, const PurchasedItem& item) = 0;
    virtual PurchasedItem getItemByTitle(const User& user, const std::string& title) = 0;
};
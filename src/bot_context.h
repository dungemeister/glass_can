#pragma once
#include <unordered_map>
#include <stdint.h>
#include <tuple>
#include <string>
#include <functional>

class BotContext{
public:
    
    enum BotState{
        INVALID,
        //Menus
        MAIN_MENU,
        STEAM_MENU,
        STEAM_PURCHASE_LIST_MENU,
        STEAM_WATCH_LIST_MENU,
        OTHER_MENU,
        //Watch list
        STEAM_WATCH_LIST_ADD_LINK,
        STEAM_WATCH_LIST_DELETE_LINK,
        STEAM_WATCH_LIST_DELETE_ALL_LINKS,
        STEAM_LIST_WATCH_LIST_LINKS,
        STEAM_INFO_WATCH_LIST_LINKS,
        //Purchase list
        STEAM_PURCHASE_LIST_ADD_LINK_TITLE,
        STEAM_PURCHASE_LIST_ADD_LINK_BUY_PRICE,
        STEAM_PURCHASE_LIST_ADD_LINK_AMOUNT,
        STEAM_PURCHASE_LIST_ADD_LINK_DATE,
        STEAM_PURCHASE_LIST_DELETE_LINK,
        //Separate Bot Commands
        SET_USER_CURRENCY,
    };
    using UserContext = std::tuple<BotState, std::string>;
    enum BotCommandTupleIndex{
        Function = 0,
        Description = 1
    };
    using BotCommand = std::tuple<std::function<void(uint64_t chat_id)>, std::string>;

    BotContext();

    void setUserContext(uint64_t chat_id, UserContext context);
    void switchState(uint64_t chat_id, BotState state);
    UserContext getUserContext(uint64_t chat_id);

private:
    
    std::unordered_map<uint64_t, UserContext> m_user_state;
};
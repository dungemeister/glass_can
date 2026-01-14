#pragma once
#include <unordered_map>
#include <stdint.h>
#include <tuple>
#include <string>

class BotContext{
public:
    
    enum BotState{
        INVALID,
        MAIN_MENU,
        STEAM_MENU,
        OTHER_MENU,
        STEAM_ADD_LINK,
        STEAM_DELETE_LINK,
        STEAM_DELETE_ALL_LINKS,
        STEAM_LIST_LINKS,
        STEAM_INFO_LINKS,
        STEAM_ADD_LINK_BUY_INFO_TITLE,
        STEAM_ADD_LINK_BUY_INFO_BUY_PRICE,
        STEAM_ADD_LINK_BUY_INFO_AMOUNT,
        STEAM_ADD_LINK_BUY_INFO_DATE,
        STEAM_DELETE_LINK_BUY_INFO,
    };
    using UserContext = std::tuple<BotState, std::string>;
    
    BotContext();

    void setUserContext(uint64_t chat_id, UserContext context);
    void switchState(uint64_t chat_id, BotState state);
    UserContext getUserContext(uint64_t chat_id);

private:
    
    std::unordered_map<uint64_t, UserContext> m_user_state;
};
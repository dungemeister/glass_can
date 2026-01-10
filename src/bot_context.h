#pragma once
#include <unordered_map>
#include <stdint.h>
#include <tuple>
#include <string>

class BotContext{
public:
    
    enum BotState{
        MAIN_MENU,
        STEAM_MENU,
        OTHER_MENU,
        STEAM_ADD_LINK,
        STEAM_DELETE_LINK,
        STEAM_LIST_LINKS,
    };
    using UserContext = std::tuple<BotState, std::string>;
    
    BotContext();

    void setUserContext(uint64_t chat_id, UserContext context);
    void switchState(uint64_t chat_id, BotState state);
    UserContext getState(uint64_t chat_id);

private:
    
    std::unordered_map<uint64_t, UserContext> m_user_state;
};
#pragma once
#include <unordered_map>
#include <stdint.h>

class BotContext{
public:
    enum BotState{
        MAIN_MENU,
        ADD_LINK,
        DELETE_LINK,
        LIST_LINKS,
    };

    BotContext();

    void switchState(uint64_t chat_id, BotState state);
    BotState getState(uint64_t chat_id);
    
private:
    std::unordered_map<uint64_t, BotState> m_user_state;
};
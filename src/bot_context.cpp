#include "bot_context.h"

BotContext::BotContext()
{}

void BotContext::setUserContext(uint64_t chat_id, BotContext::UserContext context){
    m_user_state[chat_id] = context;
}

void BotContext::switchState(uint64_t chat_id, BotState state){
    std::get<0>(m_user_state[chat_id]) = state;
}

BotContext::UserContext BotContext::getState(uint64_t chat_id){
    return m_user_state[chat_id];
}
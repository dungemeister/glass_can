#include "bot_context.h"

BotContext::BotContext()
{}

void BotContext::switchState(uint64_t chat_id, BotState state){
    m_user_state[chat_id] = state;
}

BotContext::BotState BotContext::getState(uint64_t chat_id){
    return m_user_state[chat_id];
}
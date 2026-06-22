#pragma once
#include "base_event.h"
#include <variant>
#include <vector>

struct CallbackEvent: public BaseEvent{
    std::string callback_data;
    std::string callback_query_id;
};

struct CommandEvent: public BaseEvent{
    std::string command;
    std::vector<std::string> args;
};

struct MessageEvent: public BaseEvent{
    std::string text;
};


using BotEvent = std::variant<BaseEvent, CallbackEvent, CommandEvent, MessageEvent>;
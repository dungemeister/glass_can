#pragma once
#include <string>
#include <inttypes.h>

struct BaseEvent{
    uint64_t chat_id = 0;
    uint64_t update_id = 0;
    std::string username;
};
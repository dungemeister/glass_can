#pragma once
#include <string>

struct WatchLink{
    int         chat_id;
    std::string url;
    std::string title;
    bool        enabled;

    std::string repr() const { return std::to_string(chat_id) + ": " + title + " - " + url; }
};
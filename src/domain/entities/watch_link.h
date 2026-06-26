#pragma once
#include <string>
#include <inttypes.h>

struct WatchLink{
    WatchLink(uint64_t _chat_id, const std::string& _url, const std::string& _title)
    :chat_id(_chat_id)
    ,url(_url)
    ,title(_title)
    ,enabled(true)
    {}

    WatchLink()
    :WatchLink(0, "", "") {}
    
    uint64_t    chat_id;
    std::string url;
    std::string title;
    bool        enabled;

    std::string repr() const { return std::to_string(chat_id) + ": " + title + " - " + url; }
};
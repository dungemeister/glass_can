#pragma once
#include <string>
#include "chat.h"
#include <optional>

class TelegramMessage{
public:
    TelegramMessage(TelegramChat& _chat, int _date, int _id,
     std::optional<std::string> _text = std::nullopt)
     :chat(_chat)
     ,date(_date)
     ,message_id(_id)
     ,text(_text) {}

    TelegramChat                get_chat() const        { return chat; }
    int                         get_date() const        { return date; }
    int                         get_message_id() const  { return message_id; }
    std::optional<std::string>  get_text() const        { return text; }

private:
    TelegramChat chat;
    int date;
    int message_id;
    std::optional<std::string> text;

};
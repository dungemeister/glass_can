#pragma once
#include "tg_events.h"
#include <vector>
#include <inttypes.h>

class ITelegramPort{
public:
    enum class ParseMode{
        MARKDOWN_V2,
        HTML,
    };
    enum class MessageWebPreview{
        ENABLE_PREVIEW,
        DISABLE_PREVIEW,
    };
    using inline_button_text = std::string;
    using inline_button_callback_data = std::string;
    using inline_button = std::pair<inline_button_text, inline_button_callback_data>;
    virtual ~ITelegramPort() = default;

    virtual void sendMessage(uint64_t chat_id,
                            const std::string& text,
                            const std::vector<inline_button>& inline_keyboard={},
                            ParseMode mode=ParseMode::MARKDOWN_V2,
                            MessageWebPreview web_preview=MessageWebPreview::ENABLE_PREVIEW,
                            const std::string& espace_symbols="_~>#+-=|{}.!") = 0;

    virtual void editMessage(uint64_t chat_id,
                            const std::string& text,
                            uint64_t  update_msg_id,
                            const std::vector<inline_button>& inline_keyboard={},
                            ParseMode mode=ParseMode::MARKDOWN_V2,
                            MessageWebPreview web_preview=MessageWebPreview::ENABLE_PREVIEW,
                            const std::string& espace_symbols="_~>#+-=|{}.!") = 0;

    virtual std::vector<BotEvent> getUpdates(int64_t& offset) = 0;
    
    virtual void sendMainMenu(uint64_t chat_id) = 0;
    virtual void sendSteamMainMenu(uint64_t chat_id) = 0;
    virtual void sendSteamPurchasedMenu(uint64_t chat_id) = 0;
    virtual void sendSteamWatchMenu(uint64_t chat_id) = 0;
    virtual void sendSteamSurveyMenu(uint64_t chat_id) = 0;
    virtual void sendSteamNotificationMenu(uint64_t chat_id) = 0;

    virtual void editMainMenu(uint64_t chat_id, uint64_t message_id) = 0;
    virtual void editSteamMainMenu(uint64_t chat_id, uint64_t message_id) = 0;
    virtual void editSteamPurchasedMenu(uint64_t chat_id, uint64_t message_id) = 0;
    virtual void editSteamWatchMenu(uint64_t chat_id, uint64_t message_id) = 0;
    virtual void editSteamSurveyMenu(uint64_t chat_id, uint64_t message_id) = 0;
    virtual void editSteamNotificationMenu(uint64_t chat_id, uint64_t message_id) = 0;

    //New BOT API 10.1 
    virtual void sendRichMessage(uint64_t chat_id,
                            const std::string& text,
                            const std::vector<inline_button>& inline_keyboard={},
                            ParseMode mode=ParseMode::MARKDOWN_V2,
                            MessageWebPreview web_preview=MessageWebPreview::ENABLE_PREVIEW,
                            const std::string& espace_symbols="_~>#+-=|{}.!") = 0;
};
#pragma once
#include "telegram_port.h"
#include "tg_events.h"

#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include <vector>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>
#include <mutex>

using json = nlohmann::json;
class TelegramClient: public ITelegramPort{
public:

    enum RequestType{
        GET,
        POST,
    };
    enum TgAPIRequest{
        GET_UPDATES,
        GET_ME,
        SEND_MESSAGE,
        EDIT_MESSAGE,
        FORWARD_MESSAGE,
        FORWARD_MESSAGES,
        GET_MY_COMMANDS,
        GET_FILE,
        SET_CHAT_MENU_BUTTON,
        GET_AVAILABLE_GIFTS,
        SET_MY_COMMANDS,
        SEND_RICH_MESSAGE,
    };
    

    TelegramClient(const std::string& token);
    ~TelegramClient() override {}

    void sendMessage(uint64_t chat_id,
                    const std::string& text, 
                    const std::vector<inline_button>& inline_buttons,
                    ParseMode mode,
                    MessageWebPreview web_preview,
                    const std::string& espace_symbols) override;
    void editMessage(uint64_t chat_id,
                    const std::string& text,
                    uint64_t  update_msg_id,
                    const std::vector<inline_button>& inline_buttons,
                    ParseMode mode,
                    MessageWebPreview web_preview,
                    const std::string& espace_symbols) override;

    std::vector<BotEvent> getUpdates(int64_t& offset);
    void sendMainMenu(uint64_t chat_id) override;
    void sendSteamMainMenu(uint64_t chat_id) override;
    void sendSteamPurchasedMenu(uint64_t chat_id) override;
    void sendSteamWatchMenu(uint64_t chat_id) override;
    void sendSteamSurveyMenu(uint64_t chat_id) override;
    void sendSteamNotificationMenu(uint64_t chat_id) override;

    void editMainMenu(uint64_t chat_id, uint64_t message_id) override;
    void editSteamMainMenu(uint64_t chat_id, uint64_t message_id) override;
    void editSteamPurchasedMenu(uint64_t chat_id, uint64_t message_id) override;
    void editSteamWatchMenu(uint64_t chat_id, uint64_t message_id) override;
    void editSteamSurveyMenu(uint64_t chat_id, uint64_t message_id) override;
    void editSteamNotificationMenu(uint64_t chat_id, uint64_t message_id) override;
private:
    std::unordered_map<TgAPIRequest, std::tuple<std::string, RequestType>>  m_requests_table;
    std::string m_token;
    std::mutex m_mutex;

    void init_request_table();
    json callRequest(TgAPIRequest request, const json& params);
    json callMethod(const std::string& method, RequestType type, const json& params);

    void answerCallbackQuery(const std::string& callback_id);


    void sendRichMessage(uint64_t chat_id,
                        const std::string& text,
                        const std::vector<inline_button>& inline_keyboard,
                        ParseMode mode,
                        MessageWebPreview web_preview,
                        const std::string& espace_symbols) override;
};
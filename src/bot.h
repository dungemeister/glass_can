#pragma once
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"

#include "db.h"
#include "bot_context.h"

#include "nlohmann/json.hpp"
#include <tuple>
#include <memory>

using json = nlohmann::json;

class TgBot{
public:
    enum ParseMode{
        eMARKDOWN_V2,
        eHTML,
    };

    enum TgAPIRequest{
        eGET_UPDATES,
        eGET_ME,
        eSEND_MESSAGE,
        eFORWARD_MESSAGE,
        eFORWARD_MESSAGES,
        eGET_MY_COMMANDS,
        eGET_FILE,
        eSET_CHAT_MENU_BUTTON,
        eGET_AVAILABLE_GIFTS,
    };
    enum RequestType{
        eGET,
        ePOST,
    };

    explicit TgBot(const std::string& _token)
    :m_token(_token)
    ,m_authorized(false)
    {  init_requests_table();  }
    
    void loop();
private:
    void init_requests_table();
    json callRequest(TgAPIRequest request, const json& params);
    json callMethod(const std::string& method, RequestType type, const json& params);
    
    json getUpdates(uint64_t offset);
    bool sendMessage(uint64_t chat_id, const std::string& text, const json& inline_keyboard, ParseMode mode);
    void getBotname();
    
    void getMe();
    void parseGetMeResponse(json response);

    bool forwardMessages(uint64_t chat_id, uint64_t from_chat_id, const std::vector<uint64_t>& message_ids);
    bool forwardMessage(uint64_t chat_id, uint64_t from_chat_id, uint64_t message_id);

    json getMyCommands();
    json getFile(const std::string& file_id);
    
    bool downloadTelegramFile(const std::string& file_path, const std::string& save_as);

    bool setChatMenuButton(uint64_t chat_id);
    json getAvailableGifts();

    //InlineKeyboard
    json getKeyboardForUser(uint64_t chat_id);
    json mainMenuKeyboard();
    //DataBase
    void initDatabase();

private:
    std::string m_token;
    std::string m_name;
    uint64_t    m_id;
    bool        m_authorized;
    const uint64_t m_wife_chat_id = 1305113463;
    std::unique_ptr<DataBase> m_sqlite_db;
    BotContext m_context;
    
    std::unordered_map<TgAPIRequest, std::tuple<std::string, RequestType>> m_requests_table;
};
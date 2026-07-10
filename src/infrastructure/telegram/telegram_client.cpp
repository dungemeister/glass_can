#include "telegram_client.h"
#include "message_parser.h"
#include "log_macros.h"
#include "menus.h"

#include <functional>

TelegramClient::TelegramClient(const std::string& token)
:m_token(token)
{
    init_request_table();
}

void TelegramClient::init_request_table(){
    m_requests_table = {
        {TgAPIRequest::GET_UPDATES,           {"getUpdates",            RequestType::GET}},
        {TgAPIRequest::GET_ME,                {"getMe",                 RequestType::GET}},
        {TgAPIRequest::SEND_MESSAGE,          {"sendMessage",           RequestType::POST}},
        {TgAPIRequest::EDIT_MESSAGE,          {"editMessageText",       RequestType::POST}},
        {TgAPIRequest::FORWARD_MESSAGE,       {"forwardMessage",        RequestType::POST}},
        {TgAPIRequest::FORWARD_MESSAGES,      {"forwardMessages",       RequestType::POST}},
        {TgAPIRequest::GET_MY_COMMANDS,       {"getMyCommands",         RequestType::GET}},
        {TgAPIRequest::GET_FILE,              {"getFile",               RequestType::GET}},
        {TgAPIRequest::SET_CHAT_MENU_BUTTON,  {"setChatMenuButton",     RequestType::POST}},
        {TgAPIRequest::GET_AVAILABLE_GIFTS,   {"getAvailableGifts",     RequestType::GET}},
        {TgAPIRequest::SET_MY_COMMANDS,       {"setMyCommands",         RequestType::POST}},

    };
}

std::vector<BotEvent> TelegramClient::getUpdates(int64_t& offset){
    try{
        std::vector<BotEvent> res;
        res.reserve(16);
        json params = {
            {"offset", offset}
        };
        static MessageParser parser;
        auto msgs = callRequest(TgAPIRequest::GET_UPDATES, params);
        for(auto msg: msgs){
            LOG_DEBUG(msg.dump(2));
            auto update_id = msg["update_id"].get<int64_t>();
            try{
                auto event = parser(msg);
                if(std::holds_alternative<CallbackEvent>(event)){
                    auto& event_ = std::get<CallbackEvent>(event);
                    answerCallbackQuery(event_.callback_query_id);
                }

                res.push_back(std::move(event));
                offset = update_id + 1;
            }
            catch(std::exception& e){
                LOG_ERROR(e.what());
            }            
        }
        return res;
    }
    catch(const std::exception& e){
        LOG_ERROR(e.what());
        return {};
    }
}

json TelegramClient::callRequest(TgAPIRequest request, const json& params){
    auto data = m_requests_table.find(request);
    if(data == m_requests_table.end()){
        throw std::runtime_error("Wrong request " + std::to_string(request));
    }

    auto method = std::get<0>(data->second);
    auto request_type = std::get<1>(data->second);
    
    return callMethod(method, request_type, params);
}

json TelegramClient::callMethod(const std::string& method, RequestType type, const json& params={}){
    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        std::string url;
        std::list<std::string> header;
        header.push_back("Content-Type: application/json");

        url = "https://api.telegram.org/bot" + m_token + "/" + method;

        if(type == RequestType::GET){
            //GET request
            request.setOpt(curlpp::options::HttpGet(true));
            auto temp = params.dump();
            if(!params.is_null() && !params.empty()){
                std::string query;
                for(auto& [key, value]: params.items()){
                    if(!query.empty()) query += "&";
                    std::string value_str = value.dump();
                    if(value.is_string()){
                        auto index = value_str.find("\"");
                        while(index != std::string::npos){
                            value_str.erase(index, 1);
                            index = value_str.find("\"");
                        }
                    }
                    query += key + "=" + value_str;
                    
                }

                url += "?" + query; 
            }
            if(method.find("getUpdates") == std::string::npos){
                std::cout << "Get url: " << url << std::endl;
            }
        }
        else if(type == RequestType::POST && !params.is_null() && !params.empty()){
            //POST request
            request.setOpt(curlpp::options::PostFields(params.dump().c_str()));
            request.setOpt(curlpp::options::PostFieldSize(params.dump().size()));

        }

        request.setOpt(curlpp::options::HttpHeader(header));

        request.setOpt(curlpp::options::Url(url));
        request.setOpt(curlpp::options::Timeout(30));
        request.setOpt(curlpp::options::SslVerifyPeer(false));

        std::ostringstream response;
        request.setOpt(curlpp::options::WriteStream(&response));
        {
            // std::unique_lock lock(m_mutex);
            request.perform();
        }
        
        auto doc = json::parse(response.str());
        if(!doc["ok"].get<bool>()){
            auto error = doc.value("description", "Unknown Error");
            auto error_code = doc.value("error_code", 0);
            throw std::runtime_error(method + ": Telegram API error: " + std::to_string(error_code) + "-" + error);
        }
        return doc["result"];

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()));
    }
}

void TelegramClient::answerCallbackQuery(const std::string& callback_id){
    try{
        auto res = callMethod("answerCallbackQuery", RequestType::POST, {{"callback_query_id", callback_id}});
    }
    catch(std::exception& e){
        LOG_ERROR(e.what());
    }
}

void TelegramClient::sendMessage(uint64_t chat_id,
                                const std::string& text,
                                const std::vector<inline_button>& inline_buttons={},
                                ParseMode mode=ParseMode::MARKDOWN_V2,
                                MessageWebPreview web_preview=MessageWebPreview::ENABLE_PREVIEW,
                                const std::string& espace_symbols="_~>#+-=|{}.!"){
    auto prepared_text = StringMisc::escapeString(text, espace_symbols);
    json params = {
        {"chat_id", chat_id},
        {"text", prepared_text},
        {"parse_mode", (mode == ParseMode::MARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(web_preview == MessageWebPreview::DISABLE_PREVIEW){
        params["disable_web_page_preview"] = true;
    }
    if(!inline_buttons.empty())
        params["reply_markup"] = inline_menu::BotInlineMenus::createInlineKeyboard(inline_buttons);
    try{
        LOG_DEBUG(params.dump());

        callRequest(TgAPIRequest::SEND_MESSAGE, params);
    }
    catch(const std::exception& e){
        LOG_ERROR(e.what());

    }
}

void TelegramClient::editMessage(uint64_t chat_id,
                                const std::string& text,
                                uint64_t  update_msg_id,
                                const std::vector<inline_button>& inline_buttons={},
                                ParseMode mode=ParseMode::MARKDOWN_V2,
                                MessageWebPreview web_preview=MessageWebPreview::ENABLE_PREVIEW,
                                const std::string& espace_symbols="_~>#+-=|{}.!"){

    auto prepared_text = StringMisc::escapeString(text, espace_symbols);
    json params = {
        {"chat_id",     chat_id},
        {"text",        prepared_text},
        {"message_id",  update_msg_id},
        {"parse_mode",  (mode == ParseMode::MARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(web_preview == MessageWebPreview::DISABLE_PREVIEW){
        params["disable_web_page_preview"] = true;
    }
    if(!inline_buttons.empty())
        params["reply_markup"] = inline_menu::BotInlineMenus::createInlineKeyboard(inline_buttons);
    try{
        LOG_DEBUG(params.dump());

        callRequest(TgAPIRequest::EDIT_MESSAGE, params);
    }
    catch(const std::exception& e){
        LOG_ERROR(e.what());

    }
}
void TelegramClient::sendMainMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Главное Меню", inline_menu::BotInlineMenus::getMainMenuButtons());
}
void TelegramClient::sendSteamMainMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Steam Меню", inline_menu::BotInlineMenus::getSteamMainMenuButtons());
}

void TelegramClient::sendSteamPurchasedMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Список покупок", inline_menu::BotInlineMenus::getSteamPurchasedMenuButtons());
}
void TelegramClient::sendSteamWatchMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Список отслеживания", inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
    
}
void TelegramClient::sendSteamSurveyMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Список опросов", inline_menu::BotInlineMenus::getSteamSurveyMenuButtons());
    
}
void TelegramClient::sendSteamNotificationMenu(uint64_t chat_id){
    return sendMessage(chat_id, "Список уведомлений", inline_menu::BotInlineMenus::getSteamNotificationMenuButtons());
    
}

void TelegramClient::editMainMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Главное Меню", message_id, inline_menu::BotInlineMenus::getMainMenuButtons());
}

void TelegramClient::editSteamMainMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Steam Меню", message_id, inline_menu::BotInlineMenus::getSteamMainMenuButtons());
}

void TelegramClient::editSteamPurchasedMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Список покупок", message_id, inline_menu::BotInlineMenus::getSteamPurchasedMenuButtons());
}

void TelegramClient::editSteamWatchMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Список отслеживания", message_id, inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
}

void TelegramClient::editSteamSurveyMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Список опросов", message_id, inline_menu::BotInlineMenus::getSteamSurveyMenuButtons());
}

void TelegramClient::editSteamNotificationMenu(uint64_t chat_id, uint64_t message_id){
    return editMessage(chat_id, "Список уведомлений", message_id, inline_menu::BotInlineMenus::getSteamNotificationMenuButtons());
}


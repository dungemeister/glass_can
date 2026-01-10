#include "bot.h"

#include <thread>
#include <fstream>

void TgBot::init_requests_table(){
    m_requests_table = {
        {TgAPIRequest::eGET_UPDATES,           {"getUpdates",            RequestType::eGET}},
        {TgAPIRequest::eGET_ME,                {"getMe",                 RequestType::eGET}},
        {TgAPIRequest::eSEND_MESSAGE,          {"sendMessage",           RequestType::ePOST}},
        {TgAPIRequest::eFORWARD_MESSAGE,       {"forwardMessage",        RequestType::ePOST}},
        {TgAPIRequest::eFORWARD_MESSAGES,      {"forwardMessages",       RequestType::ePOST}},
        {TgAPIRequest::eGET_MY_COMMANDS,       {"getMyCommands",         RequestType::eGET}},
        {TgAPIRequest::eGET_FILE,              {"getFile",               RequestType::eGET}},
        {TgAPIRequest::eSET_CHAT_MENU_BUTTON,  {"setChatMenuButton",     RequestType::ePOST}},
        {TgAPIRequest::eGET_AVAILABLE_GIFTS,   {"getAvailableGifts",     RequestType::eGET}},

    };
}

json TgBot::callRequest(TgAPIRequest request, const json& params={}){
    auto data = m_requests_table.find(request);
    if(data == m_requests_table.end()){
        throw std::runtime_error("Wrong request " + std::to_string(request));
    }

    auto method = std::get<0>(data->second);
    auto request_type = std::get<1>(data->second);
    
    return callMethod(method, request_type, params);
}

json TgBot::callMethod(const std::string& method, RequestType type, const json& params={}){

    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        std::string url;
        std::list<std::string> header;
        header.push_back("Content-Type: application/json");

        url = "https://api.telegram.org/bot" + m_token + "/" + method;

        if(type == RequestType::eGET){
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
        else if(type == RequestType::ePOST && !params.is_null() && !params.empty()){
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
        
        request.perform();
        
        auto doc = json::parse(response.str());
        if(!doc["ok"].get<bool>()){
            auto error = doc.value("description", "Unknown Error");
            auto error_code = doc.value("error_code", 0);
            throw std::runtime_error("Telegram API error: " + std::to_string(error_code) + "-" + error);
        }
        return doc["result"];

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()));
    }
}

json TgBot::getUpdates(uint64_t offset){
    try{
        json params = {
            {"offset", offset}
        };
        return callRequest(TgAPIRequest::eGET_UPDATES, params);
    }
    catch(const std::exception& e){
        std::cerr << "ERROR: getUpdates: " << e.what() << std::endl;
        return {};
    }
}

bool TgBot::sendMessage(uint64_t chat_id, const std::string& text, const json& inline_keyboard={}, ParseMode mode=ParseMode::eMARKDOWN_V2){
    json params = {
        {"chat_id", chat_id},
        {"text", text},
        {"parse_mode", (mode == ParseMode::eMARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(!inline_keyboard.empty())
        params["reply_markup"] = inline_keyboard;

    auto result = callRequest(TgAPIRequest::eSEND_MESSAGE, params);
    return true;
}

void TgBot::getBotname(){

}

void TgBot::getMe(){
    try{
        auto result = callRequest(TgAPIRequest::eGET_ME);
        parseGetMeResponse(result);
        m_authorized = true;
    }
    catch(const std::exception& e){
        std::cerr << "ERROR: getMe: " << e.what() << std::endl;
    }
}

void TgBot::parseGetMeResponse(json response){
    m_name = response.value("first_name", "");
    m_id   = response.value("id", 0);

    std::cout << "Bot: " << m_name << ". ID: " << m_id << std::endl;
}

void TgBot::loop(){
    std::cout << "Running SQLite Database..." << std::endl;
    initDatabase();
    std::cout << "Authorizing Bot..." << std::endl;
    getMe();
    if(!m_authorized){
        std::cerr << "Bot is not authorized" << std::endl;
        return;
    }
    
    setChatMenuButton(0);
    auto commands = getMyCommands().dump();
    auto gifts = getAvailableGifts().dump();

    uint64_t offset = 0;
    while(true){
        auto updates = getUpdates(offset);
        for(const auto& update: updates){
            offset = update["update_id"].get<uint64_t>() + 1;
            std::cout << update.dump() << std::endl;

            if(update.contains("message")){
                auto message = update["message"];
                std::string username = message["chat"]["username"];
                std::string first_name = message["chat"]["first_name"];
                uint64_t chat_id = message["chat"]["id"];

                auto user_id = m_sqlite_db->addUser(chat_id, username, first_name);
                //Handling text messages
                if(message.contains("text")){

                    auto text = message["text"].get<std::string>();
                    std::cout << "#" << chat_id << ": " << text << std::endl;
                    if(text == "ping"){
                        std::string msg = "*pong*";

                        sendMessage(chat_id, msg);
                    }
                    if(text == "/start"){
                        m_context.switchState(chat_id, BotContext::BotState::MAIN_MENU);

                        auto inline_keyboard = getKeyboardForUser(chat_id);
                        sendMessage(chat_id, "Hello, @" + username, inline_keyboard);
                    }
                    
                }
                //Handling voice messages
                if(message.contains("voice")){
                    auto voice = message["voice"];

                    auto file_id   = voice["file_id"].get<std::string>();
                    auto file_size = voice["file_size"].get<uint64_t>();
                    auto duration  = voice["duration"].get<uint64_t>();

                    std::cout << "file id: " << file_id << std::endl;
                    std::cout << "file size: " << file_size << std::endl;
                    std::cout << "duration: " << duration << std::endl;
                    if(file_size <= 20 * 1024 * 1024){
                        auto result = getFile(file_id);
                        if(!result.empty() && !result.is_null()){
                            auto file_path = result["file_path"];
                            auto file_size = result["file_size"];

                            auto res = downloadTelegramFile(file_path, username + "_voice.ogg");
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool TgBot::forwardMessages(uint64_t chat_id, uint64_t from_chat_id, const std::vector<uint64_t>& message_ids){
    try{

        nlohmann::json ids = nlohmann::json::array();
        for (auto id : message_ids) ids.push_back(id);

        json params = {
            {"chat_id", chat_id},
            {"from_chat_id", from_chat_id},
            {"message_ids", std::move(ids)}
        };

        return callRequest(TgAPIRequest::eFORWARD_MESSAGES, params);
    }
    catch (const std::exception& e){
        std::cerr << "ERROR: forwardMessages: " << e.what() << std::endl;
        return false;
    }
}

bool TgBot::forwardMessage(uint64_t chat_id, uint64_t from_chat_id, uint64_t message_id){
    try{
        json params = {
            {"chat_id", chat_id},
            {"from_chat_id", from_chat_id},
            {"message_ids", message_id}
        };

        return callRequest(TgAPIRequest::eFORWARD_MESSAGE, params);
    }
    catch (const std::exception& e){
        std::cerr << "ERROR: forwardMessage: " << e.what() << std::endl;
        return false;
    }
}

json TgBot::getMyCommands(){
    try{
        return callRequest(TgAPIRequest::eGET_MY_COMMANDS);
    }
    catch (const std::exception& e){
        std::cerr << "ERROR: getMyCommands: " << e.what() << std::endl;
        return {};
    }
}

json TgBot::getFile(const std::string& file_id){
    try{
        json params = {
            {"file_id", file_id},
        };

        return callRequest(TgAPIRequest::eGET_FILE, params);
    }
    catch (const std::exception& e){
        std::cerr << "ERROR: getFile: " << e.what() << std::endl;
        return {};
    }
}

bool TgBot::downloadTelegramFile(const std::string& file_path, const std::string& save_as){
    try{
        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        std::string url;

        url = "https://api.telegram.org/file/bot" + m_token + "/" + file_path;

        request.setOpt(curlpp::options::Url(url));
        request.setOpt(curlpp::options::Timeout(30));
        request.setOpt(curlpp::options::SslVerifyPeer(false));
        request.setOpt(curlpp::options::ConnectTimeout(10));
        request.setOpt(curlpp::options::NoSignal(true));

        std::ofstream file_stream(save_as, std::ios::binary | std::ios::trunc);
        if (!file_stream.is_open()) {
            std::cerr << "Fail to open file " << save_as << std::endl;
            return false;
        }

        request.setOpt(curlpp::options::WriteStream(&file_stream));
        
        request.perform();
        
        file_stream.seekp(0, std::ios::end);
        size_t downloaded_size = file_stream.tellp();

        std::cout << "Downloaded size: " << downloaded_size << std::endl;
        return downloaded_size > 0;
    }

    catch(const std::exception& e){
        throw std::runtime_error(std::string(e.what()));
        return false;
    }
}

bool TgBot::setChatMenuButton(uint64_t chat_id){
    try{
        json params = {
            {"menu_button", {{"type", "default"}}}
        };
        if(chat_id > 0){
            params["chat_id"] = chat_id;
        }

        callRequest(TgAPIRequest::eSET_CHAT_MENU_BUTTON, params);
        return true;
    }
    catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return false;
    }
}

void TgBot::initDatabase(){
    m_sqlite_db = std::make_unique<DataBase>("steam.db");
}

json TgBot::getAvailableGifts(){
    try{
        return callRequest(TgAPIRequest::eGET_AVAILABLE_GIFTS);
    }
    catch(const std::exception& e){
        std::cerr << "getAvailableGifts: " << e.what() << std::endl;
        return {};
    }
}

json TgBot::mainMenuKeyboard(){
    return {
            {"inline_keyboard", {
                { {{ "text", "Отслеживаемый список steam"},     {"callback_data", "steam_list"}} ,
                 {{ "text", "Добавить в steam список"},      {"callback_data", "steam_add"}} },
                { {{ "text", "Удалить из steam списка"},   {"callback_data", "steam_delete"}} },
            }}
    };
}

json TgBot::getKeyboardForUser(uint64_t chat_id){
    auto state = m_context.getState(chat_id);
    switch(state){
        case BotContext::BotState::MAIN_MENU:
            return mainMenuKeyboard();
        
        case BotContext::BotState::ADD_LINK:
            return {};
        
        case BotContext::BotState::DELETE_LINK:
            return {};
        
        case BotContext::BotState::LIST_LINKS:
            return {};
    }
}
#include "bot.h"
#include "misc.h"

#include <thread>
#include <fstream>
void TgBot::initRequestsTable(){
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
            throw std::runtime_error(method + ": Telegram API error: " + std::to_string(error_code) + "-" + error);
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
    
    auto prepared_text = StringMisc::escapeString(text);
    json params = {
        {"chat_id", chat_id},
        {"text", prepared_text},
        {"parse_mode", (mode == ParseMode::eMARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(!inline_keyboard.empty())
        params["reply_markup"] = inline_keyboard;
    try{
        callRequest(TgAPIRequest::eSEND_MESSAGE, params);
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "sendMessage: " << e.what() << std::endl;
        return false;
    }
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

void TgBot::handleText(uint64_t chat_id, const std::string& text){
    auto user_context = m_context.getState(chat_id);
    auto user_state = std::get<0>(user_context);
    auto username = std::get<1>(user_context);

    std::cout << "#" << username << ": " << text << std::endl;
    if(text == "ping"){
        std::string msg = "*pong*";

        sendMessage(chat_id, msg);
    }
    if(text == "/start"){
        m_context.switchState(chat_id, BotContext::BotState::MAIN_MENU);

        auto inline_keyboard = getMenuForUser(chat_id);
        sendMessage(chat_id, "Hello, @" + username, inline_keyboard);
    }
    switch(user_state){
        case BotContext::BotState::STEAM_ADD_LINK:
            if(auto res = addSteamLink(chat_id, text); !res){
                sendMessage(chat_id, "Ошибка добавления", steamMenu());
            }
            else{
                sendMessage(chat_id, "Ссылка добавлена", steamMenu());
            }
        break;

        case BotContext::BotState::STEAM_DELETE_LINK:
            if(auto res = deleteSteamLink(chat_id, text); !res){
                sendMessage(chat_id, "Ошибка удаления", steamMenu());
            }
            else{
                sendMessage(chat_id, "Ссылка удалена", steamMenu());
            }
        break;
    }
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
            // std::cout << update.dump() << "\n" << std::endl;

            if(update.contains("message")){
                auto message = update["message"];
                std::string username = message["chat"]["username"];
                std::string first_name = message["chat"]["first_name"];
                auto chat_id = message["chat"]["id"].get<uint64_t>();

                auto user_state = m_context.getState(chat_id);
                if(std::get<0>(user_state) == BotContext::BotState::INVALID){
                    m_context.setUserContext(chat_id, std::make_tuple(BotContext::BotState::MAIN_MENU, username));
                }
                auto user_id = m_sqlite_db->addUser(chat_id, username, first_name);
                //Handling text messages
                if(message.contains("text")){
                    auto text = message["text"].get<std::string>();
                    handleText(chat_id, text);
                    
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
            if(update.contains("callback_query")){
                handleCallbackQuery(update["callback_query"]);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
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

json TgBot::mainMenu(){
    return {
            {"inline_keyboard", {
                { {{ "text", "Steam список"},                   {"callback_data", c_steam_menu_string}} },
                { {{ "text", "Другой список"},                  {"callback_data", "other_menu"}} },
            }}
    };
}


json TgBot::getMenuForUser(uint64_t chat_id){
    auto context = m_context.getState(chat_id);
    auto state = std::get<0>(context);

    switch(state){
        case BotContext::BotState::MAIN_MENU:
            return mainMenu();

        case BotContext::BotState::STEAM_MENU:
            return steamMenu();

        case BotContext::BotState::OTHER_MENU:
            return {};

        case BotContext::BotState::STEAM_ADD_LINK:
            return {};
        
        case BotContext::BotState::STEAM_DELETE_LINK:
            return {};
        
        case BotContext::BotState::STEAM_LIST_LINKS:
            return {};
    }
}

json TgBot::steamMenu(){
    return {
            {"inline_keyboard", {
                { {{ "text", "Отобразить список"},              {"callback_data", c_steam_list_string}} ,
                  {{ "text", "Добавить в список"},              {"callback_data", c_steam_add_string}} },

                { {{ "text", "Текущая инфа списка"},            {"callback_data", c_steam_info_string}},
                  {{ "text", "Удалить из списка"},              {"callback_data", c_steam_delete_string}}},

                { {{ "text", "В главное меню"},                 {"callback_data", c_main_menu_string}} },
            }}
            };
}

json TgBot::steamAddLinkMenu(){
    return{
            {"inline_keyboard", {
                { {{"text", "Отмена"},         {"callback_data", c_steam_menu_string}} },
                { {{"text", "В главное меню"}, {"callback_data", c_main_menu_string}} }
            }}
        };
}

void TgBot::handleCallbackQuery(const json& callback){
    try{
        auto chat_id = callback["from"]["id"].get<uint64_t>();
        auto message_id = callback["message"]["message_id"].get<uint64_t>();
        if(callback.contains("data")){
            auto data = callback["data"];
            auto res = callMethod("answerCallbackQuery", RequestType::ePOST, {
                                    {"callback_query_id", callback["id"]}
                                });
            if(data.get<std::string>() == c_main_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Main Menu*"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", mainMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::MAIN_MENU);
            }
            if(data.get<std::string>() == c_steam_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Steam Menu*"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
                
            }
            if(data.get<std::string>() == c_steam_list_string){
                auto links = m_sqlite_db->getUserLinks(chat_id);
                for(auto& link: links){
                    std::stringstream out;
                    out << link["title"] << ": " << link["url"] << std::endl;
                    std::cout << out.str();
                    sendMessage(chat_id, out.str());
                }
                m_context.switchState(chat_id, BotContext::BotState::STEAM_LIST_LINKS);
                sendMessage(chat_id, "*Steam Menu*", steamMenu());
            }
            if(data.get<std::string>() == c_steam_add_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Steam Add Link Menu*\n Отправь ссылку для отслеживания в формате 'https://example\\.com \\- CS2 Spectrum case'"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamAddLinkMenu()}

                            });
                
                m_context.switchState(chat_id, BotContext::BotState::STEAM_ADD_LINK);
                
            }
            if(data.get<std::string>() == c_steam_info_string){
                auto links = m_sqlite_db->getUserLinks(chat_id);
                for(const auto& link: links){
                    auto temp = link["url"].get<std::string>();
                    auto index = temp.rfind("/");
                    const std::string item_hash_name =  temp.substr(index + 1);
                    auto res = PriceOverview::Parser::getSteamItemPrice(c_steam_app_id, item_hash_name, c_steam_currency);
                    auto data = json::parse(res);

                    std::stringstream out;
                    if(data["success"].get<bool>()){

                        // std::string currency = (c_steam_currency == PriceOverview::SteamCurrency::eUSD)?"$":"Rub";
                        std::string currency = "";

                        out << "*" << item_hash_name << "*\n" <<
                                "Начальная цена на продажу: *" << data["lowest_price"] << currency << "*\n" <<
                                "Медианная цена: *" << data["median_price"] << currency << "*\n" <<
                                "Объем лотов: *" << data["volume"] << "*" << std::endl;
                    }
                    else{
                        out << "*" << item_hash_name << "*." << "Ошибка выполнения запроса: " << data["error"] << std::endl;
                    }
                    sendMessage(chat_id, out.str());
                }
                sendMessage(chat_id, "*Steam Menu*", steamMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);

            }
            if(data.get<std::string>() == c_steam_delete_string){
                auto links = m_sqlite_db->getUserLinks(chat_id);
                if(links.size() <= 0){
                    sendMessage(chat_id, "Список пустой", steamMenu());
                    m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
                }
                else{

                    json titles_menu = {{"inline_keyboard", { {  }, } }};
                    for(auto& link: links){
                        std::stringstream out;
                        out << link["title"] << ": " << link["url"] << std::endl;
                        std::cout << out.str();
                        sendMessage(chat_id, out.str());
                    }

                    sendMessage(chat_id, "Выбери какую ссылку удалить", steamMenu());
                    m_context.switchState(chat_id, BotContext::BotState::STEAM_DELETE_LINK);
                }

            }
        }
    }
    catch(const std::exception& e){
        std::cerr << "handleCallbackQuery: " << e.what() << std::endl;
    }
}

bool TgBot::addSteamLink(uint64_t chat_id, const std::string& line){
    auto sep_index = line.find("-");
    auto link = line.substr(0, sep_index - 1);
    auto title = line.substr(sep_index + 2);
    std::cout << "Adding to " << chat_id << " " << link << " " << title << std::endl;
    return m_sqlite_db->addUserLink(chat_id, link, title);
}

bool TgBot::deleteSteamLink(uint64_t chat_id, const std::string& title){
    std::cout << "Deleting from " << chat_id << " " << title << std::endl;
    return m_sqlite_db->deleteUserLink(chat_id, title);
}
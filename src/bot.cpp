#include "bot.h"
#include "string_misc.h"
#include "filesystem_manager.h"

#include <thread>
#include <fstream>
#include <iomanip>
#include <string>

void TgBot::initRequestsTable(){
    m_requests_table = {
        {TgAPIRequest::eGET_UPDATES,           {"getUpdates",            RequestType::eGET}},
        {TgAPIRequest::eGET_ME,                {"getMe",                 RequestType::eGET}},
        {TgAPIRequest::eSEND_MESSAGE,          {"sendMessage",           RequestType::ePOST}},
        {TgAPIRequest::eEDIT_MESSAGE,          {"editMessageText",       RequestType::ePOST}},
        {TgAPIRequest::eFORWARD_MESSAGE,       {"forwardMessage",        RequestType::ePOST}},
        {TgAPIRequest::eFORWARD_MESSAGES,      {"forwardMessages",       RequestType::ePOST}},
        {TgAPIRequest::eGET_MY_COMMANDS,       {"getMyCommands",         RequestType::eGET}},
        {TgAPIRequest::eGET_FILE,              {"getFile",               RequestType::eGET}},
        {TgAPIRequest::eSET_CHAT_MENU_BUTTON,  {"setChatMenuButton",     RequestType::ePOST}},
        {TgAPIRequest::eGET_AVAILABLE_GIFTS,   {"getAvailableGifts",     RequestType::eGET}},
        {TgAPIRequest::eSET_MY_COMMANDS,       {"setMyCommands",         RequestType::ePOST}},

    };
}

void TgBot::initCommandList(){
    m_commands_map = {
        {"/start", {
                    [this](uint64_t chat_id){ sendMessage(chat_id, "Выбери список", mainMenu(), ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");},
                    "Команда для сброса состояния бота"}
        },

        {"/ping" , {
                    [this](uint64_t chat_id){ sendMessage(chat_id, "pong"         , {}        , ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");},
                    "Проверка доступности бота"}
        },
        {"/setcurrency" , {
                    [this](uint64_t chat_id){
                    sendMessage(chat_id, "Введи код валюты: USD - 1, EUR - 3, RUB - 5", {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "-.,");
                    m_context.switchState(chat_id, BotContext::BotState::SET_USER_CURRENCY);
                    },
                    "Установка валюты пользователя"}
        },
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

void TgBot::handleUpdate(const json& update){
    if(update.contains("message")){
        auto message = update["message"];
        std::string username = message["chat"]["username"];
        std::string first_name = message["chat"]["first_name"];
        auto chat_id = message["chat"]["id"].get<uint64_t>();

        auto user_state = m_context.getUserContext(chat_id);
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

bool TgBot::sendMessage(uint64_t chat_id, const std::string& text, const json& inline_keyboard={},
                        ParseMode mode=ParseMode::eMARKDOWN_V2, MessageWebPreview web_preview=eENABLE_WEB_PREVIEW,
                        const std::string& espace_symbols="_~>#+-=|{}.!"){
    
    auto prepared_text = StringMisc::escapeString(text, espace_symbols);
    std::cout << prepared_text << std::endl;
    json params = {
        {"chat_id", chat_id},
        {"text", prepared_text},
        {"parse_mode", (mode == ParseMode::eMARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(web_preview == MessageWebPreview::eDISABLE_WEB_PREVIEW){
        params["disable_web_page_preview"] = true;
    }
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

bool TgBot::editMessageText(uint64_t chat_id, uint64_t message_id, const std::string& text, const json& inline_keyboard={},
                        ParseMode mode=ParseMode::eMARKDOWN_V2, MessageWebPreview web_preview=eENABLE_WEB_PREVIEW,
                        const std::string& espace_symbols="_~>#+-=|{}.!"){

    auto prepared_text = StringMisc::escapeString(text, espace_symbols);
    std::cout << prepared_text << std::endl;
    json params = {
        {"chat_id", chat_id},
        {"message_id", message_id},
        {"text", prepared_text},
        {"parse_mode", (mode == ParseMode::eMARKDOWN_V2)? "MarkdownV2":"HTML"},
    };
    if(web_preview == MessageWebPreview::eDISABLE_WEB_PREVIEW){
        params["disable_web_page_preview"] = true;
    }
    if(!inline_keyboard.empty())
        params["reply_markup"] = inline_keyboard;
    try{
        callRequest(TgAPIRequest::eEDIT_MESSAGE, params);
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "editMessageText: " << e.what() << std::endl;
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
    auto user_context = m_context.getUserContext(chat_id);
    auto user_state = std::get<0>(user_context);
    auto username = std::get<1>(user_context);
    auto first_char = text.at(0);

    if(first_char == '/'){
        auto it = m_commands_map.find(text);
        if(it == m_commands_map.end()){
            sendMessage(chat_id, "Неизвестная команда");
        }
        else{
            auto handler = std::get<BotContext::BotCommandTupleIndex::Function>(it->second);
            handler(chat_id);
        }
        return;
    }

    std::cout << "#" << username << ": " << text << std::endl;
    
    switch(user_state){
        case BotContext::BotState::STEAM_WATCH_LIST_ADD_LINK:
            if(auto res = addSteamLink(chat_id, text); !res){
                sendMessage(chat_id, "Ошибка добавления", steamMenu());
            }
            else{
                sendMessage(chat_id, "Ссылка добавлена", steamMenu());
            }
            m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
        break;

        case BotContext::BotState::STEAM_WATCH_LIST_DELETE_LINK:
            if(auto res = deleteSteamLink(chat_id, text); !res){
                sendMessage(chat_id, "Ошибка удаления", steamMenu());
            }
            else{
                sendMessage(chat_id, "Ссылка удалена", steamMenu());
            }
            m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
        break;

        case BotContext::BotState::STEAM_PURCHASE_LIST_ADD_LINK_BUY_PRICE:
        {
            auto& info = m_user_buy_item_info[chat_id];
            try{
                std::stringstream price_ss;
                price_ss << text;
                price_ss >> m_user_buy_item_info[chat_id].buy_price;

                sendMessage(chat_id, "Информация добавлена");
                json keyboard;
                keyboard["inline_keyboard"] = json::array();
                keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_menu_string}} });
                sendMessage(chat_id,
                            "Введи купленное количество предметов по цене " + std::to_string(m_user_buy_item_info[chat_id].buy_price),
                            keyboard);
                m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_ADD_LINK_AMOUNT);

            }
            catch(...){
                sendMessage(chat_id, "Ошибка добавления информации", steamMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);

            }

        }
        break;

        case BotContext::BotState::STEAM_PURCHASE_LIST_ADD_LINK_AMOUNT:
        {
            auto& info = m_user_buy_item_info[chat_id];
            try{
                info.user_link = StringMisc::sqlQuoteShielding(info.user_link);
                info.title = StringMisc::sqlQuoteShielding(info.title);

                std::stringstream amount_ss;
                amount_ss << text;
                amount_ss >> m_user_buy_item_info[chat_id].amount;

                if(auto res = m_sqlite_db->addUserItemBuyInfo(chat_id, info); !res){
                    sendMessage(chat_id, "Ошибка добавления информации. DB error", steamMenu());
                }
                else{

                    sendMessage(chat_id, "Информация о покупках добавлена", steamMenu());
                }
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);

            }
            catch(...){
                sendMessage(chat_id, "Ошибка добавления информации", steamMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);

            }
        }
        break;
        case BotContext::BotState::SET_USER_CURRENCY:
        {
            auto val = std::atoi(text.c_str());
            if(auto cur = ENUM_VALID_CAST(PriceOverview::SteamCurrency, val)){
                auto cur_str = getSteamCurrencyString(*cur);
                auto res = m_sqlite_db->setUserCurrency(chat_id, cur_str);
                if(res["ok"].get<bool>()){
                    sendMessage(chat_id, "Валюта " + cur_str + " установлена", steamMenu());
                }
                else{
                    sendMessage(chat_id, "Ошибка: " + res["error_msg"].get<std::string>(), steamMenu());
                }
            }
            else{
                sendMessage(chat_id, "Ошибка: невалидное значение валюты " + std::to_string(val), steamMenu());
            }
            m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
        }
        break;
        default: break;
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
    
    auto commands = getMyCommands();
    updateBotCommands(commands);
    WorkerPool::Task upload_task = [&,this](){
        this->uploadSurveyTasks();
    };
    m_workers_pool.enqueue(upload_task);

    auto gifts = getAvailableGifts().dump();
    uint64_t offset = 0;
    while(true){
        auto updates = getUpdates(offset);
        for(const auto& update: updates){
            offset = update["update_id"].get<uint64_t>() + 1;
            m_workers_pool.enqueue([this, update]{
                handleUpdate(update);
            });

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

json TgBot::setMyCommands(){
    try{
        json updated_commands = json::array();
        for(auto& bot_command: m_commands_map){
            auto& cmd_name = bot_command.first;
            auto& cmd_description = std::get<BotContext::BotCommandTupleIndex::Description>(bot_command.second);

            updated_commands.push_back({{"command", cmd_name}, {"description", cmd_description}});
        }

        json params = {
                        {"commands", updated_commands}
                    };
        return callRequest(TgAPIRequest::eSET_MY_COMMANDS, params);

    }
    catch(const std::exception& e){
        std::cerr << "ERROR: setMyCommands: " << e.what() << std::endl;
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

bool TgBot::uploadTelegramPhoto(const std::string file_path){

    return false;
}

std::string TgBot::sendLocalFile(const std::string& token, long long chat_id, const std::string& file_path, const std::string& caption, const std::string& method){
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("File not found: " + file_path);
    }
    
    curlpp::Cleanup cleanup;
    curlpp::Easy request;
    
    std::string url = "https://api.telegram.org/bot" + token + "/" + method;
    
    //TODO:
    throw std::runtime_error("sendLocalFile not implemented yet");
    
    try {
        request.setOpt(curlpp::options::Url(url));
        request.setOpt(curlpp::options::Post(true));
        // request.setOpt(curlpp::options::PostFields(multipart_body.str()));
        // request.setOpt(curlpp::options::HttpHeader(headers));
        
        std::ostringstream response;
        request.setOpt(curlpp::options::WriteStream(&response));
        
        request.perform();
        return response.str();
        
    } catch (curlpp::RuntimeError& e) {
        throw std::runtime_error("curlpp: " + std::string(e.what()));
    }
}

std::string TgBot::sendPhotoFile(long long chat_id,  const std::string& file_path, const std::string& caption = "") {
    
    std::filesystem::path path(file_path);
    if (!std::filesystem::exists(path)) {
        return "{\"ok\":false,\"error\":\"File not found\"}";
    }
    
    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        std::ostringstream response;
        
        request.setOpt(curlpp::options::Url("https://api.telegram.org/bot" + m_token + "/sendPhoto"));
        request.setOpt(curlpp::options::Timeout(30));
        request.setOpt(curlpp::options::Verbose(false));
        
        curlpp::Forms formParts;
        formParts.push_back(new curlpp::FormParts::Content("chat_id", std::to_string(chat_id)));
        formParts.push_back(new curlpp::FormParts::Content("parse_mode", "MarkdownV2"));
        formParts.push_back(new curlpp::FormParts::File("photo", file_path));
        
        if (!caption.empty()) {
            formParts.push_back(new curlpp::FormParts::Content("caption", caption));
        }
        
        request.setOpt(curlpp::options::HttpPost(formParts));
        request.setOpt(curlpp::options::WriteStream(&response));
        
        request.perform();
        
        return response.str();;
        
    } catch (curlpp::RuntimeError& e) {
        return "{\"ok\":false,\"error\":\"curlpp: " + std::string(e.what()) + "\"}";
    } catch (curlpp::LogicError& e) {
        return "{\"ok\":false,\"error\":\"curlpp logic: " + std::string(e.what()) + "\"}";
    } catch (std::exception& e) {
        return "{\"ok\":false,\"error\":\"" + std::string(e.what()) + "\"}";
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
    m_sqlite_db = std::make_unique<DataBase>("db/steam.db");
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
                { { { "text", "🎮Steam список"},                   {"callback_data", c_steam_menu_string} } },
                // { { { "text", "🔥Другой список"},                  {"callback_data", "other_menu"} } },
            }}
    };
}

json TgBot::steamMenu(){
    return {
            {"inline_keyboard", {

                { {{ "text", "🛒Список покупок"},                   {"callback_data", c_steam_purchase_list_menu_string}} },
                { {{ "text", "👀Список отслеживания"},              {"callback_data", c_steam_watch_list_menu_string}} },
                { {{ "text", "⏱️Список опросов"},                   {"callback_data", c_steam_survey_list_menu_string}} },
                { {{ "text", "⏰Список уведомлений"},               {"callback_data", c_steam_notification_list_menu_string}} },
                { {{ "text", "🔄В главное меню"},                   {"callback_data", c_main_menu_string}} },
            }}
            };
}

json TgBot::steamWatchListMenu(){
    return{
            {"inline_keyboard", {
                { {{ "text", "📋Отобразить список"},                {"callback_data", c_steam_list_watch_list_string}} ,
                  {{ "text", "➕Добавить в список"},                {"callback_data", c_steam_add_watch_list_string}} },

                { {{ "text", "📈Актуальные цены"},                  {"callback_data", c_steam_info_watch_list_string}},
                  {{ "text", "➖Удалить из списка"},                {"callback_data", c_steam_delete_watch_list_string}}},
                { {{"text", "🔄В главное меню"},                    {"callback_data", c_steam_menu_string}} }
            }}
        };
}

json TgBot::steamPurchaseListMenu(){
    return{
            {"inline_keyboard", {
                { {{ "text", "📋Отобразить список"},                {"callback_data", c_steam_purchased_item_info_string}} },
                
                { {{ "text", "➕Добавить данные по закупке"},       {"callback_data", c_steam_add_purchased_item_string}},
                 {{ "text", "➖Удалить данные по закупке"},         {"callback_data", c_steam_delete_purchased_item_string}}},

                { {{ "text", "📈Данные по закупке"},                {"callback_data", c_steam_list_purchased_items_string}}},
                { {{"text", "🔄В главное меню"},                    {"callback_data", c_steam_menu_string}} }
            }}
        };
}

json TgBot::steamWatchListAddLinkMenu(){
    return{
            {"inline_keyboard", {
                { {{"text", "❌Отмена"},         {"callback_data", c_steam_list_watch_list_string}} },
                { {{"text", "🔄В главное меню"}, {"callback_data", c_steam_watch_list_menu_string}} }
            }}
        };
}

json TgBot::steamSurveyListMenu(){
    return {
                {"inline_keyboard",{
                    { {{"text", "📋Отобразить список"},                 {"callback_data", c_steam_survey_list_list_string}} },

                    { {{ "text", "➕Добавить данные по закупке"},       {"callback_data", c_steam_survey_list_add_string}},
                      {{ "text", "➖Удалить данные по закупке"},        {"callback_data", c_steam_survey_list_delete_string}}},
                    { {{"text", "🔄В главное меню"},                    {"callback_data", c_steam_menu_string}} },
                }}
            };
}

void TgBot::handleCallbackQuery(const json& callback){
    try{
        auto chat_id = callback["from"]["id"].get<uint64_t>();
        auto message_id = callback["message"]["message_id"].get<uint64_t>();
        auto user_context = m_context.getUserContext(chat_id);
        auto user_bot_state = std::get<0>(user_context);
        auto username = std::get<1>(user_context);

        if(callback.contains("data")){
            auto& data = callback["data"];
            auto res = callMethod("answerCallbackQuery", RequestType::ePOST, {
                                    {"callback_query_id", callback["id"]}
                                });
            //Menus
            
            //Send main menu
            if(data.get<std::string>() == c_main_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Главное Меню*"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", mainMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::MAIN_MENU);
                return;
            }
            //Send Steam purchase list menu
            else if(data.get<std::string>() == c_steam_purchase_list_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Список закупок*"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamPurchaseListMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
                return;
            }
            //Send Steam watch list menu
            else if(data.get<std::string>() == c_steam_watch_list_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "*Список отслеживания*"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamWatchListMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_MENU);
                return;
            }

            //Send steam menu to user
            else if(data.get<std::string>() == c_steam_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "Меню стим списков"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::STEAM_MENU);
                if(m_user_buy_item_info.count(chat_id) > 0){
                    m_user_buy_item_info.erase(chat_id);
                    std::cout << chat_id << ": erased buy item info after cancel" << std::endl;
                }
                return;
            }
            //Send Survey List Menu to user
            else if(data.get<std::string>() == c_steam_survey_list_menu_string){
                callMethod("editMessageText", RequestType::ePOST, {
                            {"chat_id", chat_id},
                            {"message_id", message_id},
                            {"text", "Меню списка периодических опросов"},
                            {"parse_mode", "MarkdownV2"},
                            {"reply_markup", steamSurveyListMenu()}

                            });
                m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_MENU);
                return;
            }

            // Steam watch list
            //Get user links list from Steam watch list
            else if(data.get<std::string>() == c_steam_list_watch_list_string){
                getWatchListItemsList(chat_id);
            }
            //Send message to add link to Steam watch list
            else if(data.get<std::string>() == c_steam_add_watch_list_string){
                sendWatchListAddLinkMessage(chat_id, message_id);
            }
            //Send current items info from Steam watch list
            else if(data.get<std::string>() == c_steam_info_watch_list_string){
                getWatchListItemsData(chat_id);
            }

            //Send menu to delete from Steam watch list
            else if(data.get<std::string>() == c_steam_delete_watch_list_string){
                sendWatchListDeleteMenu(chat_id);
            }
            
            //Steam purchased list
            //Send menu to add item to user's Steam purchased list
            else if(data.get<std::string>() == c_steam_add_purchased_item_string){
                sendPurchasedItemsMenu(chat_id);
            }
            //Send list of items from user's purchased list
            else if(data.get<std::string>() == c_steam_list_purchased_items_string){
                getPurchasedItemsData(chat_id);
            }
            else if(data.get<std::string>() == c_steam_purchased_item_info_string){
                getPurchasedItemsList(chat_id);
            }
            else if(data.get<std::string>() == c_steam_delete_purchased_item_string){
                deletePurchasedItem(chat_id);
            }
            else if(data.get<std::string>() == c_steam_survey_list_add_string){
                sendSurveyListAddMenu(chat_id, message_id);
                // addPeriodicTask(chat_id, "survey_steam_task");
            }
            else if(data.get<std::string>() == c_steam_survey_list_delete_string){
                // deletePeriodicTask(chat_id, 0);
                sendSurveyListDeleteMenu(chat_id);
            }
            else if(data.get<std::string>() == c_steam_survey_list_list_string){
                getSurveyListUserLinks(chat_id, -1);
            }
        }
        //Send menu to delete user's item from purchased list
        if(user_bot_state == BotContext::BotContext::STEAM_WATCH_LIST_DELETE_LINK){
            if(callback.contains("data")){
                std::string data = callback["data"];
                data = StringMisc::escapeString(data, "_~>#+-=|{}.!()");
                auto& callback_id = callback["id"];
                auto res = callMethod("answerCallbackQuery", RequestType::ePOST, {
                                    {"callback_query_id", callback_id}
                                });
                if(auto res = m_sqlite_db->deleteUserLink(chat_id, data); !res){
                    sendMessage(chat_id, "Ошибка удаления " + data, steamWatchListMenu());
                }
                else{
                    sendMessage(chat_id, "Элемент '" + data + "' удален", steamWatchListMenu());
                }
                m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_MENU);
            }
            return;
        }
        else if(user_bot_state == BotContext::BotContext::STEAM_PURCHASE_LIST_ADD_LINK_TITLE){
            if(callback.contains("data")){
                std::string data = callback["data"];
                auto& callback_id = callback["id"];
                auto res = callMethod("answerCallbackQuery", RequestType::ePOST, {
                                    {"callback_query_id", callback_id}
                                });
                
                m_user_buy_item_info[chat_id].chat_id = chat_id;
                m_user_buy_item_info[chat_id].title = data;

                json keyboard;
                keyboard["inline_keyboard"] = json::array();
                keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_purchase_list_menu_string}} });
                sendMessage(chat_id, "Введи цену покупки в $",  keyboard);
                m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_ADD_LINK_BUY_PRICE);
            }
            return;
        }
        else if(user_bot_state == BotContext::BotContext::STEAM_PURCHASE_LIST_DELETE_LINK)
        {
            std::string data = callback["data"];
            std::cout << data << std::endl;
            auto tokens = StringMisc::splitByDelim(data, '|');
            std::stringstream out;
            out << std::fixed << std::setprecision(2) << std::stof(tokens[3]);
            float buy_value = std::stof(out.str());
            int amount = std::atoi(tokens[2].c_str());

            auto res = m_sqlite_db->deleteUserItemBuyInfo(chat_id, tokens[1], buy_value, amount);
            if(res["ok"]){

                sendMessage(chat_id, "OK", steamPurchaseListMenu());
            }
            else {
                sendMessage(chat_id, res["error_msg"], steamPurchaseListMenu());

            }
            m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
        }
        else if(user_bot_state == BotContext::BotContext::STEAM_SURVEY_LIST_ADD_LINK){
            std::string data = callback["data"];
            std::string& title = data;
            sendSurveyListAddLinkPeriodMenu(chat_id, title, message_id);

        }
        else if(user_bot_state == BotContext::BotContext::STEAM_SURVEY_LIST_SET_LINK_PERIOD){
            std::string data = callback["data"];
            std::cout << data << std::endl;
            auto payload = StringMisc::splitOnceByDelim(data, ' ', StringMisc::SplitOnce::FROM_END);
            
            std::string title;
            for(auto it = payload.begin(); it != payload.end() - 1; ++it){
                title += *it;
            }
            int period_min = 0;
            if(payload.size() > 1)
                period_min = std::atoi(payload[payload.size() - 1].c_str());
            else
                throw std::runtime_error("Fail to parse callback data " + data);
            auto link = m_sqlite_db->getUserLinkByTitle(chat_id, title);

            auto res = m_sqlite_db->addUserSurveyLink(chat_id, title, link["url"], period_min);
            if(res["ok"]){
                std::cout << link << std::endl;
                SurveyLink survey_link( link["url"].get<std::string>(),
                                        link["title"].get<std::string>(),
                                        period_min,
                                        chat_id);

                auto task = createSurveyTask(survey_link);
                addPeriodicTask(std::move(task));

                editMessageText(chat_id, message_id, "Ссылка добавлена в список опроса", steamSurveyListMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_MENU);
            }
            else{
                std::string error_msg = StringMisc::escapeString(res["error_msg"].get<std::string>(), "_~>#+-=|{}.!()");
                nlohmann::json keyboard;
                keyboard["inline_keyboard"].push_back({ {{ "text", "❌В меню"},                 {"callback_data", c_steam_survey_list_menu_string}} });
                editMessageText(chat_id, message_id, "Ошибка добавления в список опроса " + error_msg, keyboard);
            }
        }
        else if(user_bot_state == BotContext::BotContext::STEAM_SURVEY_LIST_DELETE_LINK){
            std::string data = callback["data"];
            std::cout << data << std::endl;
            auto res = m_sqlite_db->deleteSurveyLink(chat_id, data);
            if(res["ok"]){
                sendMessage(chat_id, "Успешно удалено " + data, steamSurveyListMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
            }
            else{
                sendMessage(chat_id, "ERROR: " + res["error_msg"].get<std::string>(), steamSurveyListMenu());
                m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
            }
        }
    }
        
    
    catch(const std::exception& e){
        std::cerr << "handleCallbackQuery: " << e.what() << std::endl;
    }
}

bool TgBot::addSteamLink(uint64_t chat_id, const std::string& line){
    auto sep_index = line.find("#");
    auto link = StringMisc::sqlQuoteShielding(line.substr(0, sep_index - 1));
    auto title = StringMisc::sqlQuoteShielding(line.substr(sep_index + 2));
    std::cout << "Adding to " << chat_id << " " << link << " " << title << std::endl;
    return m_sqlite_db->addUserLink(chat_id, link, title);
}

bool TgBot::deleteSteamLink(uint64_t chat_id, const std::string& title){
    std::cout << "Deleting from " << chat_id << " " << title << std::endl;
    return m_sqlite_db->deleteUserLink(chat_id, title);
}

json TgBot::getUserLinkPriceOverview(const std::string& link_url){
    auto url = link_url;
    auto index = url.rfind("/");
    const std::string item_hash_name =  url.substr(index + 1);
    auto res = PriceOverview::Parser::getSteamItemPrice(c_steam_app_id, item_hash_name, c_steam_currency);
    auto data = json::parse(res);
    json result;

    std::stringstream out;
    auto markdown_link = StringMisc::createMarkdownLink(url, StringMisc::uriToString(item_hash_name));
    if(data["success"].get<bool>()){

        // std::string currency = (c_steam_currency == PriceOverview::SteamCurrency::eUSD)?"$":"Rub";
        std::string currency = "";

        out <<  "Начальная цена на продажу: *" << data["lowest_price"] << currency << "*\n" <<
                "Медианная цена: *" << data["median_price"] << currency << "*\n" <<
                "Объем лотов: *" << data["volume"] << "*" << std::endl;
        result["ok"] = true;
        result["json"] = data;
    }
    else{
        out << "*" << item_hash_name << "*." << "Ошибка выполнения запроса: " << data["error"] << std::endl;
        result["ok"] = false;
    }
    result["caption"] = StringMisc::uriToString(item_hash_name);
    result["url"] = url;
    result["data"] = out.str();
    result["json"] = data;

    return result;
}

std::string TgBot::convertUserLinkMinimal(const json& link){
    auto& url = link["url"];
    auto& title = link["title"];
    return StringMisc::createMarkdownLink(url, title);
}

nlohmann::json TgBot::createInlineKeyboard(const std::vector<std::pair<std::string, std::string>>& buttons,
                                   const std::string& callback_prefix = "btn_",
                                   int columns = 2) {
    
    nlohmann::json keyboard = {{"inline_keyboard", nlohmann::json::array()}};
    auto& keyboard_rows = keyboard["inline_keyboard"];
    
    for (size_t i = 0; i < buttons.size(); i += columns) {
        nlohmann::json row = nlohmann::json::array();
        
        for (int j = 0; j < columns && (i + j) < buttons.size(); ++j) {
            std::string text = buttons[i + j].first;
            std::string callback_data;
            if(!buttons[i+j].second.empty())
                callback_data = callback_prefix + buttons[i+j].second;
            else 
                callback_data = callback_prefix + text;
            
            // std::replace(callback_data.begin(), callback_data.end(), ' ', '_');
            
            row.push_back(nlohmann::json{
                {"text", text},
                {"callback_data", callback_data}
            });
        }
        
        keyboard_rows.push_back(row);
    }
    
    return keyboard;
}

std::string TgBot::getUserItemChart(const json& link){
    try{
        auto market_hash_name = StringMisc::getUriNameFromSteamLink(link["url"].get<std::string>());
        auto item_name = StringMisc::uriToString(market_hash_name);
        auto time_period = PriceHistory::eTimePeriod::MONTH;

        std::stringstream assets_dir;
        assets_dir << "charts/" << item_name;
        auto filesystem_res = FilesystemManager::rm_rf(assets_dir.str() + "/*");
        filesystem_res = FilesystemManager::mkdir(assets_dir.str());
        
        auto plots = PriceHistory::getPriceHistory(market_hash_name, time_period, static_cast<uint64_t>(c_steam_currency));
        std::cout << "For " << item_name << " found points: " << plots.size() << std::endl;

        std::string points_data_file = assets_dir.str() + "/" + market_hash_name + "_data";
        std::string png_file = assets_dir.str() + "/" + market_hash_name + "_" + PriceHistory::getTimePeriodString(time_period) + ".png";
        std::string gnuplot_script_file = assets_dir.str() + "/" + market_hash_name + "_" + PriceHistory::getTimePeriodString(time_period) +".gp";

        auto res = GnuplotChart::createChartPngImage(plots, points_data_file, gnuplot_script_file, png_file, time_period);
        auto abs_path = FilesystemManager::abs_path(png_file);
        return abs_path;
    }
    catch(const std::exception& e){
        std::cerr << "ERROR: getUserItemChart: " << e.what() << std::endl;
        return {};
    }
}

std::string TgBot::getUserItemPriceAnalysys(const json& link, const json& price){
    std::stringstream ss;
    auto temp1 = price.dump();
    auto temp2 = link.dump();
    float cur_price = 0, buy_price = 0, profit_price = 0;
    uint64_t amount = 0;
    switch(c_steam_currency){
        case PriceOverview::SteamCurrency::eUSD:
        {
            auto cur_price_ = price["lowest_price"].get<std::string>();
            if(auto index = cur_price_.find("$"); index != std::string::npos)
                cur_price_.erase(index, 1);

            cur_price = std::stof(cur_price_);
            amount = std::stoi(link["amount"].get<std::string>());
            buy_price = std::stof(link["buy_price"].get<std::string>());
            profit_price = buy_price * 1.15; //Steam comission is 15%

        }
        break;
        default: break;
    }
    auto markdown_link = StringMisc::createMarkdownLink(link["url"], link["title"]);

    ss << std::fixed << std::setprecision(2) << 
          "Цена покупки: *" << buy_price << "*\n" <<
          "Количество: *" << amount << "*\n" <<
          "Текущая цена: *" << cur_price << "*\n" <<
          "Цена для окупаемости: *"  << profit_price << "*\n" <<
          "-----------------------------------------\n" << 
          "Текущий профит ($ с учётом комиссии): *" << (cur_price - profit_price) * amount << "$*\n" <<
          "Текущий профит (% с учетом комиссии): *" << static_cast<float>((100 * (cur_price - profit_price)) / buy_price) << "%*";
    auto prepared_out = StringMisc::escapeString(ss.str(), "().,-");
    return markdown_link + "\n" + prepared_out;
}

void TgBot::updateBotCommands(const json& commands){
    bool update = false;
    for(auto& bot_command: m_commands_map){
        auto& defined_cmd = bot_command.first;
        bool found = false;
        for(auto& req_command: commands){
            if(auto requested_cmd = req_command.value("name", ""); requested_cmd != ""){
                std::cout << "Found command " << defined_cmd << std::endl;
                found = true;
                break;
            }

        }
        if(!found){
            std::cout << "Command " << defined_cmd << " not found" << std::endl;
            update = true;
            break;
        }
    }
    if(update){
        setMyCommands();
    }
}

void TgBot::deletePurchasedItem(int chat_id){
    auto db_res = m_sqlite_db->getUserItemsBuyInfo(chat_id);
    auto& links = db_res["data"];
    std::vector<std::pair<std::string, std::string>> buttons;
    int counter = 0;
    for(auto& link: links){
        std::stringstream out;
        out << counter++ << "|" << link["title"] << "|"
                        << link["amount"] << "|" 
                        << link["buy_price"] << std::endl;
        // std::string title = link["title"];
        buttons.emplace_back(StringMisc::removeQuotes(out.str()), "");
    }
    auto keyboard = createInlineKeyboard(buttons, "", 1);
    keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_purchase_list_menu_string}} });
    sendMessage(chat_id, "Выбери какую ссылку удалить", keyboard);
    // sendMessage(chat_id, "*Steam Menu*", steamMenu());
    m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_DELETE_LINK);
}

void TgBot::getPurchasedItemsList(int chat_id){
    std::cout << "Requested purchased items info" << std::endl;
    auto db_res = m_sqlite_db->getUserItemsBuyInfo(chat_id);
    auto& links = db_res["data"];
    if(db_res["ok"].get<bool>()){
        int counter = 0;
        for(auto& link: links){
            std::stringstream out;

            out << counter++ << ". " << link["title"] << "| "
                << "*" << link["amount"] << "*" << "шт| " 
                << "*" << link["buy_price"] << "*" << std::endl;
            std::string prepared_str = StringMisc::escapeString(out.str(), "_~>#+-=|{}.!()");
            prepared_str = StringMisc::removeQuotes(prepared_str);
            sendMessage(chat_id, prepared_str, {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");

        }
        sendMessage(chat_id, "*Список покупок*", steamPurchaseListMenu(), ParseMode::eMARKDOWN_V2);
    }
    else {
        sendMessage(chat_id, "Ошибка: " + db_res["error_msg"].get<std::string>(), steamPurchaseListMenu());
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
}

void TgBot::getPurchasedItemsData(int chat_id){
    auto db_res = m_sqlite_db->getUserItemsBuyInfo(chat_id);
    auto& links = db_res["data"];
    
    if(db_res["ok"].get<bool>()){
        if(links.size() < 0){
            sendMessage(chat_id, "Список о запупках пуст", steamPurchaseListMenu());
            m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
            return;
        }
        for(auto& link: links){
            std::cout << "\t" << link["url"] << ": " << link["title"] << std::endl;
            auto price_res = getUserLinkPriceOverview(link["url"].get<std::string>());
            auto temp = price_res.dump();
            if(price_res["json"]["lowest_price"].is_null()){

                sendMessage(chat_id, "Ошибка запроса: " + link["url"].get<std::string>(), {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW);                            continue;
            }
            auto out = getUserItemPriceAnalysys(link, price_res["json"]);
            sendMessage(chat_id, out, {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");
        }
        sendMessage(chat_id, "*Список покупок*", steamPurchaseListMenu(), ParseMode::eMARKDOWN_V2);
    }
    else{
        sendMessage(chat_id, "Ошибка: " + db_res["error_msg"].get<std::string>(), steamPurchaseListMenu());
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
}

void TgBot::sendPurchasedItemsMenu(int chat_id){
    auto links = m_sqlite_db->getUserLinks(chat_id);
    if(links.size() <= 0){
        sendMessage(chat_id, "Список пустой", steamPurchaseListMenu());
        m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_MENU);
    }
    else{
        std::vector<std::pair<std::string, std::string>> buttons;
        for(auto& link: links){
            std::string title = link["title"];
            buttons.emplace_back(title, "");
        }
        auto keyboard = createInlineKeyboard(buttons, "", 2);
        keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_purchase_list_menu_string}} });
        sendMessage(chat_id, "Выбери ссылку для добавления информации", keyboard);
        // sendMessage(chat_id, "*Steam Menu*", steamMenu());
        m_context.switchState(chat_id, BotContext::BotState::STEAM_PURCHASE_LIST_ADD_LINK_TITLE);
    }
}

void TgBot::sendWatchListDeleteMenu(int chat_id){
    auto links = m_sqlite_db->getUserLinks(chat_id);
    if(links.size() <= 0){
        sendMessage(chat_id, "Список пустой", steamWatchListMenu());
        m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_MENU);
    }
    else{
        std::vector<std::pair<std::string, std::string>> buttons;
        for(auto& link: links){
            std::string title = link["title"];
            buttons.emplace_back(title, "");
        }
        auto keyboard = createInlineKeyboard(buttons, "", 2);
        keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_watch_list_menu_string}} });
        sendMessage(chat_id, "Выбери какую ссылку удалить", keyboard);
        // sendMessage(chat_id, "*Steam Menu*", steamMenu());
        m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_DELETE_LINK);
    }
}

void TgBot::getWatchListItemsData(int chat_id){

    auto links = m_sqlite_db->getUserLinks(chat_id);
    if(links.size() > 0){
        for(const auto& link: links){
            auto markdown_link = StringMisc::createMarkdownLink(link["url"], StringMisc::escapeString(StringMisc::uriToString(link["title"])));
            auto result = getUserLinkPriceOverview(link["url"].get<std::string>());
            auto item_name = StringMisc::escapeString(result["caption"].get<std::string>(), "-(){}|.,=");
            try {
                auto png_path = getUserItemChart(link);

                std::stringstream caption;
                caption << "График \"" << item_name << "\"\n" <<
                            markdown_link << "\n" <<
                            StringMisc::escapeString(result["data"]) << std::endl;
                if(png_path.empty()){
                    std::cerr << "ERROR: Fail to get png_file path" << std::endl;
                    sendMessage(chat_id, caption.str(), {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");
                    continue;
                }
                auto response = sendPhotoFile(chat_id, png_path, caption.str());
                auto json = nlohmann::json::parse(response);
                if (json["ok"].get<bool>()) {
                    std::cout << "Photo sent with file_id: " << json["result"]["photo"][0]["file_id"] << std::endl;
                } else {
                    std::cerr << "Error: " << json["description"] << std::endl;
                }
            } catch (...) {
                std::cerr << " JSON парсинг ошибка" << std::endl;
            }
        }
        sendMessage(chat_id, "*Список наблюдения*", steamWatchListMenu());
    }
    else{

        sendMessage(chat_id, "*Список наблюдения пуст*", steamWatchListMenu());
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_MENU);
}

void TgBot::sendWatchListAddLinkMessage(int chat_id, int message_id){

    std::string text = "*Steam Add Link Menu*\n Отправь ссылку для отслеживания в формате 'https://example.com # CS2 Spectrum case'";

    auto res = editMessageText(chat_id, message_id, text, steamWatchListAddLinkMenu());

    if(res){
        m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_ADD_LINK);
    }
    else{
        m_context.switchState(chat_id, BotContext::BotState::STEAM_WATCH_LIST_ADD_LINK);
        sendMessage(chat_id, "*Ошибка запроса*", steamWatchListMenu());
    }
}

void TgBot::getWatchListItemsList(int chat_id){
    auto links = m_sqlite_db->getUserLinks(chat_id);
                
    size_t index = 1;
    std::stringstream out;
    out << "🎮Steam список:\n";
    for(const auto& link: links){
        out << index++ << " - " << convertUserLinkMinimal(link) << std::endl;
    }
    // auto out = StringMisc::createMarkdownLinkTable(links);
    m_context.switchState(chat_id, BotContext::BotState::STEAM_LIST_WATCH_LIST_LINKS);
    sendMessage(chat_id, out.str() + "*Steam Menu*", steamWatchListMenu(), ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW);
}

void TgBot::addPeriodicTask(PeriodicTasks::PeriodicTaskDescriptor&& task){
    std::cout << "Added task " << task.name << std::endl;
    m_periodic_pool.addTask(std::move(task));
}

void TgBot::deletePeriodicTask(int chat_id, uint64_t id){
    m_periodic_pool.stopTask(id);
    std::cout << "Stopped task " << id << std::endl;
}

void TgBot::sendSurveyListAddMenu(int chat_id, int message_id=-1){
    auto links = m_sqlite_db->getUserLinks(chat_id);
    std::vector<std::pair<std::string, std::string>> buttons;
    for(auto& link: links){
        std::string title = link["title"];
        buttons.emplace_back(title, "");
    }
    auto keyboard = createInlineKeyboard(buttons, "", 2);
    keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_survey_list_menu_string}} });

    if (message_id <= 0){
        sendMessage(chat_id, "Меню добавления", keyboard);
    }
    else{
        editMessageText(chat_id, message_id, "Меню добавления", keyboard);
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_ADD_LINK);
}

void TgBot::sendSurveyListAddLinkPeriodMenu(int chat_id, const std::string& title, int message_id=-1){
    std::vector<std::pair<std::string, std::string>> buttons = {{"5мин" , "5" },
                                                                {"30мин", "30"},
                                                                {"60мин", "60"},
                                                                {"12ч"  , "720"},
                                                                {"24ч"  , "1440"},
                                                                {"48ч"  , "2880"}};
    auto keyboard = createInlineKeyboard(buttons, title + " ", 2);
    keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_survey_list_menu_string}} });
    if (message_id <= 0){
        sendMessage(chat_id, "Выбери период опроса", keyboard);
    }
    else{
        editMessageText(chat_id, message_id, "Выбери период опроса", keyboard);
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_SET_LINK_PERIOD);
}

void TgBot::sendSurveyListDeleteMenu(int chat_id){
    // auto links = m_sqlite_db->getUserLinks(chat_id);
    auto res = m_sqlite_db->getUserSurveyLinks(chat_id, (DataBasePagination){});
    if(res["ok"]){
        auto links = res["data"];
        std::vector<std::pair<std::string, std::string>> buttons;
        for(auto& link: links){
            SurveyLink survey_link(link);
            buttons.emplace_back(survey_link.title, "");
        }
        auto keyboard = createInlineKeyboard(buttons, "", 2);
        keyboard["inline_keyboard"].push_back({ {{ "text", "❌Отмена"},                 {"callback_data", c_steam_survey_list_menu_string}} });
        sendMessage(chat_id, "Выбери позицию для удаления", keyboard);
        m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_DELETE_LINK);
    }
    else {
        sendMessage(chat_id, "Меню списка опроса", steamSurveyListMenu());
        m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_MENU);
    }
}

void TgBot::getSurveyListUserLinks(int chat_id, int message_id=-1){
    DataBasePagination pag = {
        .offset = 0,
        .limit = 0,
    };
    std::stringstream out;
    auto res = m_sqlite_db->getUserSurveyLinks(chat_id, pag);
    if(res["ok"]){
        auto& links = res["data"];
        size_t row_index = 0;
        std::cout << links << std::endl;
        for(auto& link: links){
            SurveyLink survey_link(link);
            out << ++row_index << ". " <<
                   StringMisc::createMarkdownLink(survey_link.url, survey_link.title) << " " <<
                   std::to_string(survey_link.period) << "\n";
        }
        sendMessage(chat_id, "Список ссылок для опроса:\n" + StringMisc::removeQuotes(out.str()), {},
                    TgBot::ParseMode::eMARKDOWN_V2, TgBot::MessageWebPreview::eDISABLE_WEB_PREVIEW);
        if (message_id <= 0){
            sendMessage(chat_id, "Меню списка опроса", steamSurveyListMenu());
        }
        else{
            editMessageText(chat_id, message_id, "Меню списка опроса", steamSurveyListMenu());
        }
    }
    else{
        sendMessage(chat_id, "Ошибка получения списка " + res["error_msg"].get<std::string>(), steamSurveyListMenu());
    }
    m_context.switchState(chat_id, BotContext::BotState::STEAM_SURVEY_LIST_MENU);
}

void TgBot::uploadSurveyTasks(){
    DataBasePagination pag = {.offset = 0, .limit = 0,};
    auto res = m_sqlite_db->getUsers(pag);
    auto& users = res["data"];

    if(res["ok"]){
        for(auto& user: users){
            auto username = user["username"].get<std::string>();
            auto chat_id = static_cast<uint64_t>(std::atoi(user["chat_id"].get<std::string>().c_str()));
            auto links_res = m_sqlite_db->getUserSurveyLinks(chat_id, (DataBasePagination){});
            if(links_res["ok"]){
                auto links = links_res["data"];
                for(auto link: links){
                    std::cout << link << std::endl;
                    auto survey_link = SurveyLink(link);
                    auto task = createSurveyTask(survey_link);
                    addPeriodicTask(std::move(task));
                }
            }
            else{
                std::cout << "WARNING " << username << " " << chat_id << ":" <<
                             links_res["error_msg"] << std::endl;
            }
        }

    }
    else{
        std::cout << "WARNING: " << res["error_msg"].get<std::string>() << std::endl;
    }

}

PeriodicTasks::PeriodicTaskDescriptor TgBot::createSurveyTask(const SurveyLink& link){
    std::string task_name = std::to_string(link.chat_id) + " " + link.title;
    std::function<void()> task_action = [link, this](){
        auto res = this->getUserLinkPriceOverview(link.url);
        if(res["ok"]){
            std::stringstream out;
            out << StringMisc::createMarkdownLink(link.url, link.title) << "\n";
            auto data = res["data"].get<std::string>();
            out << data;
            sendMessage(link.chat_id, StringMisc::escapeString(out.str()), {}, ParseMode::eMARKDOWN_V2, MessageWebPreview::eDISABLE_WEB_PREVIEW, "");
        }
    };
    PeriodicTasks::PeriodicTaskDescriptor task = {
        .id = m_periodic_pool.getTaskSize(),
        .name = task_name,
        .period = link.period * 60 * 1000,
        .next_run = PeriodicTasks::Clock::now(),
        .paused = false,
        .stopped = false,
        .action = task_action,
        .chat_id = link.chat_id,
    };
    return std::move(task);
}
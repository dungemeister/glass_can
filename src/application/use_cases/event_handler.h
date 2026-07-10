#pragma once
#include "tg_events.h"
#include "telegram_port.h"
#include "menus.h"
#include "user_irepository.h"
#include "watch_link_irepository.h"
#include "log_macros.h"
#include "string_misc.h"

#include "price_overview_parser.h"

#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

class EventHandler{
public:    
    using CommandHandler = std::function<void(uint64_t)>;
    using ResponseCallback = std::function<void(const std::string& response_text)>;

    EventHandler(std::shared_ptr<ITelegramPort> tg_client,
                 std::shared_ptr<IUserRepository> user_rep,
                 std::shared_ptr<IWatchLinkRepository> watch_rep)
    :m_tg_client(tg_client)
    ,m_user_rep(user_rep)
    ,m_watch_rep(watch_rep)
    {
        m_commands.emplace("/start", [this](uint64_t chat_id){
            m_tg_client->sendMainMenu(chat_id);
        });
        m_commands.emplace("/ping", [this](uint64_t chat_id){
            m_tg_client->sendMessage(chat_id, "pong");
        });
        m_commands.emplace("/setcurrency", [this](uint64_t chat_id){
            m_tg_client->sendMessage(chat_id, "3");
        });
    }


    void execute(const BotEvent& event, ResponseCallback callback){
        std::visit([&callback, this](const auto& concrete) {
            using T = std::decay_t<decltype(concrete)>;

            if constexpr (std::is_same_v<T, MessageEvent>) {
                callback(concrete.username + ": " + concrete.text);
                handleMessageEvent(concrete);

            }
            else if constexpr (std::is_same_v<T, CallbackEvent>){
                callback(concrete.username + ": " + concrete.callback_data);
                callbackStateHandler(concrete, concrete.callback_data);
                // m_tg_client.sendMessage(concrete.chat_id, concrete.callback_data);
            }
            else if constexpr (std::is_same_v<T, CommandEvent>){
                std::stringstream ss;
                for(auto& arg: concrete.args){
                    ss << arg << " ";
                }
                callback(concrete.username + ": " + concrete.command + " -" + ss.str());
                if(auto it = m_commands.find(concrete.command); it != m_commands.end()){
                    it->second(static_cast<uint64_t>(concrete.chat_id));
                }
                else{
                    m_tg_client->sendMessage(static_cast<uint64_t>(concrete.chat_id), "Неизвестная команда");
                }
            }
            else{
                callback("Unknown event");
            }

        }, event);
        
    }
private:
    std::shared_ptr<ITelegramPort> m_tg_client;
    std::shared_ptr<IUserRepository> m_user_rep;
    std::shared_ptr<IWatchLinkRepository> m_watch_rep;
    std::unordered_map<std::string, CommandHandler> m_commands;

    void callbackStateHandler(CallbackEvent event, const std::string& callback_data){
        // 1.Получаем текущий статус пользователя из БД
        auto user_opt = m_user_rep->getUser(event.chat_id);
        if(!user_opt.has_value()){
            LOG_ERROR("Fail to handle user callback data. User not found in DB");
            return;
        }
        // 2.Обрабатываем данные в соответствие со статусом
        auto& user = user_opt.value();
        auto args = StringMisc::splitByDelim(callback_data, ' ');
        auto& command = args[0];

        // Handling steam menu buttons for different lists
        if(callback_data == inline_menu::c_steam_menu_cb_data){
            m_tg_client->sendSteamMainMenu(event.chat_id);
            if(user.get_state() != "normal_work"){
                set_user_state(user, "normal_work");
            }
        }
        else if(callback_data == inline_menu::c_steam_purchase_list_menu_cb_data){
            m_tg_client->editSteamPurchasedMenu(event.chat_id, event.message_id);
            if(user.get_state() != "normal_work"){
                set_user_state(user, "normal_work");
            }
        }
        else if(callback_data == inline_menu::c_steam_watch_list_menu_cb_data){
            m_tg_client->editSteamWatchMenu(event.chat_id, event.message_id);
            if(user.get_state() != "normal_work"){
                set_user_state(user, "normal_work");
            }
        }
        else if(callback_data == inline_menu::c_steam_survey_list_menu_cb_data){
            m_tg_client->editSteamSurveyMenu(event.chat_id, event.message_id);
            if(user.get_state() != "normal_work"){
                set_user_state(user, "normal_work");
            }
        }
        else if(callback_data == inline_menu::c_steam_notification_list_menu_cb_data){
            m_tg_client->editSteamNotificationMenu(event.chat_id, event.message_id);
            if(user.get_state() != "normal_work"){
                set_user_state(user, "normal_work");
            }
        }
        //WatchList handling
        else if(callback_data == inline_menu::c_steam_watch_list_list_cb_data){
            auto links = m_watch_rep->getLinks(user);
            auto links_qty = links.size();
            LOG_INFO("User links: " + std::to_string(links_qty));
            sendUserWatchListItems(user, event);
        }
        else if(callback_data == inline_menu::c_steam_watch_list_add_cb_data){
            try{
                std::vector<std::pair<std::string, std::string>> buttons;
                buttons.emplace_back("❌Отмена", inline_menu::c_steam_watch_list_menu_cb_data);

                set_user_state(user, "wl_waiting_url");
                m_tg_client->sendMessage(event.chat_id,
                                        "Введи steam url в формате \\(url : title\\). Например, https://steamcommunity.com/market/listings/730/G18EE253004 : Гремучий кейс\n"
                                        "[' : ' является разделителем]",
                                        buttons,
                                        ITelegramPort::ParseMode::MARKDOWN_V2,
                                        ITelegramPort::MessageWebPreview::DISABLE_PREVIEW);
            }
            catch(std::exception& e){
                LOG_ERROR(e.what());
                m_tg_client->sendMessage(event.chat_id,
                                        "Ошибка добавления",
                                        inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
                set_user_state(user, "normal_work");
            }
        }
        else if(command == inline_menu::c_watch_list_delete_prefix){
            auto link_title = StringMisc::splitOnceByDelim(callback_data, ' ')[1];
            WatchLink watch_link;
            try{
                watch_link = m_watch_rep->getLinkFromTitle(user, link_title);
                m_watch_rep->deleteLink(watch_link);
                m_tg_client->editMessage(event.chat_id,
                                        "Ссылка " + watch_link.title + " удалена",
                                        event.message_id,
                                        inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
            }
            catch(std::exception& e){
                LOG_ERROR(e.what());
                m_tg_client->editMessage(event.chat_id,
                                        "Ссылка " + watch_link.title + " не удалена. Ошибка.",
                                        event.message_id,
                                        inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
            }
        }
        else if(callback_data == inline_menu::c_steam_watch_list_delete_list_cb_data){
            try{
                
                std::vector<std::pair<std::string, std::string>> buttons;
                auto links = m_watch_rep->getLinks(user);
                for(auto& link: links){
                    buttons.emplace_back(link.title, inline_menu::c_watch_list_delete_prefix + " " + link.title);
                }
                buttons.emplace_back("❌Отмена", inline_menu::c_steam_watch_list_menu_cb_data);

                m_tg_client->editMessage(event.chat_id,
                                        "Выбери предмет для удаления из списка отслеживания",
                                        event.message_id,
                                        buttons);
            }
            catch(std::exception& e){
                LOG_ERROR(e.what());
                m_tg_client->sendMessage(event.chat_id,
                                        "Ошибка построения списка удаления",
                                        inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
            }
        }
        else if(callback_data == inline_menu::c_steam_watch_list_info_cb_data){
            try{
                
                auto links = m_watch_rep->getLinks(user);
                std::string msg;
                for(auto& link: links){
                    msg = StringMisc::createMarkdownLink(link.url, link.title);
                    msg += "\n";
                    msg += getWatchLinkPriceOverview(link);
                    m_tg_client->sendMessage(event.chat_id,
                                            msg);
                }
                m_tg_client->sendSteamWatchMenu(event.chat_id);
            }
            catch(std::exception& e){
                LOG_ERROR(e.what());
                m_tg_client->sendMessage(event.chat_id,
                                        "Ошибка построения списка удаления",
                                        inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
            }
        }
    }

    void sendUserWatchListItems(const User& user, const CallbackEvent& event){
        auto links = m_watch_rep->getLinks(user);
                
        size_t index = 1;
        std::stringstream out;
        out << "🎮Steam список:\n";
        for(const auto& link: links){
            out << index++ << " - " << convertUserLinkMinimal(link) << std::endl;
        }
        m_tg_client->editMessage(event.chat_id,
                                out.str(),
                                event.message_id,
                                inline_menu::BotInlineMenus::getSteamWatchMenuButtons(),
                                ITelegramPort::ParseMode::MARKDOWN_V2,
                                ITelegramPort::MessageWebPreview::DISABLE_PREVIEW);
    }

    std::string convertUserLinkMinimal(const WatchLink& link){
        return StringMisc::createMarkdownLink(link.url, link.title);
    }

    void handleMessageEvent(MessageEvent event){
        auto user_opt = m_user_rep->getUser(event.chat_id);
        if(!user_opt.has_value()){
            LOG_ERROR("FAIL to get user from db with caht_id: " + std::to_string(event.chat_id));
            return;
        }
        auto& user = user_opt.value();
        auto user_state = user.get_state();
        if(user_state == "normal_work"){
            m_tg_client->sendMessage(event.chat_id, "Неизвестная команда. Воспользуйся меню");
            m_tg_client->sendMainMenu(event.chat_id);
        }
        else if(user_state == "wl_waiting_url"){
            set_user_state(user, "normal_work");

            std::regex delim_pattern("\\s:\\s");
            auto user_data = StringMisc::splitByRegex(event.text, delim_pattern);
            if(user_data.size() < 2){
                LOG_ERROR("FAIL to split user (url : title) from '" + event.text + "'. Returning");
                m_tg_client->sendMessage(event.chat_id, "Ошибка добавления ссылки в список отслеживания.", inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
                return;
            } 
            auto& url = user_data[0];
            auto& title = user_data[1];

            WatchLink watch_link(event.chat_id, url, title);
            m_watch_rep->addLink(watch_link);

            m_tg_client->sendMessage(event.chat_id, "Ссылка '" + title + "' добавлена.", inline_menu::BotInlineMenus::getSteamWatchMenuButtons());
        }
    }

    void set_user_state(User& user, const std::string& state){
        user.set_state(state);
        m_user_rep->save(user);
    }

    std::string getWatchLinkPriceOverview(const WatchLink& link){
        std::stringstream out_msg;
        auto index = link.url.rfind("/");
        const std::string item_hash_name = link.url.substr(index + 1);
        auto res = PriceOverview::Parser::getSteamItemPrice("730", item_hash_name);
        LOG_DEBUG(res);
        auto res_json = json::parse(res);

        if(res_json["success"].get<bool>()){
            out_msg <<  "Начальная цена на продажу: *" << res_json["lowest_price"] << "*\n" <<
                        "Медианная цена: *" << res_json["median_price"] << "*\n" <<
                        "Объем лотов: *" << res_json["volume"] << "*" << std::endl; 
        }
        else{
            LOG_ERROR(res_json["error"].get<std::string>());
            out_msg << "*" << link.title << "*. Ошибка выполнения запроса";
        }
        return out_msg.str();
    }
};

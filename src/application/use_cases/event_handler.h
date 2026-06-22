#pragma once
#include "tg_events.h"
#include "telegram_port.h"
#include "menus.h"
#include "user_irepository.h"
#include "watch_link_irepository.h"
#include "log_macros.h"
#include "string_misc.h"

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
                m_tg_client->sendMessage(static_cast<uint64_t>(concrete.chat_id), concrete.text,
                                        {},
                                        ITelegramPort::ParseMode::MARKDOWN_V2,
                                        ITelegramPort::MessageWebPreview::ENABLE_PREVIEW,
                                        "_~>#+-=|{}.!"
                                        );
            }
            else if constexpr (std::is_same_v<T, CallbackEvent>){
                callback(concrete.username + ": " + concrete.callback_data);
                callbackStateHandler(concrete.chat_id, concrete.callback_data);
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

    void callbackStateHandler(uint64_t chat_id, const std::string& callback_data){
        // 1.Получаем текущий статус пользователя из БД
        auto user_opt = m_user_rep->getUser(chat_id);
        if(!user_opt.has_value()){
            LOG_ERROR("Fail to handle user callback data. User not found in DB");
            return;
        }
        // 2.Обрабатываем данные в соответствие со статусом
        auto& user = user_opt.value();
        // Handling steam menu buttons for different lists
        if(callback_data == inline_menu::c_steam_menu_cb_data){
            m_tg_client->sendSteamMainMenu(chat_id);
        }
        else if(callback_data == inline_menu::c_steam_purchase_list_menu_cb_data){
            m_tg_client->sendSteamPurchasedMenu(chat_id);
        }
        else if(callback_data == inline_menu::c_steam_watch_list_menu_cb_data){
            m_tg_client->sendSteamWatchMenu(chat_id);
        }
        else if(callback_data == inline_menu::c_steam_survey_list_menu_cb_data){
            m_tg_client->sendSteamSurveyMenu(chat_id);
        }
        else if(callback_data == inline_menu::c_steam_notification_list_menu_cb_data){
            m_tg_client->sendSteamNotificationMenu(chat_id);
        }

        else if(callback_data == inline_menu::c_steam_watch_list_list_cb_data){
            auto links = m_watch_rep->getLinks(user);
            auto links_qty = links.size();
            LOG_INFO("User links: " + std::to_string(links_qty));
            m_tg_client->sendMessage(chat_id, "Количество строк " + std::to_string(links_qty));
            sendUserWatchListItems(user);
        }
        // 3.Отвечаем новым меню или сообщением
        // m_tg_client->sendMessage(chat_id, "OK da");
    }

    void sendUserWatchListItems(const User& user){
        auto links = m_watch_rep->getLinks(user);
                
        size_t index = 1;
        std::stringstream out;
        out << "🎮Steam список:\n";
        for(const auto& link: links){
            out << index++ << " - " << convertUserLinkMinimal(link) << std::endl;
        }
        m_tg_client->sendMessage(user.get_chat_id(),
                                out.str(),
                                inline_menu::BotInlineMenus::getSteamWatchMenuButtons(),
                                ITelegramPort::ParseMode::MARKDOWN_V2,
                                ITelegramPort::MessageWebPreview::DISABLE_PREVIEW);
    }

    std::string convertUserLinkMinimal(const WatchLink& link){
        return StringMisc::createMarkdownLink(link.url, link.title);
    }
};

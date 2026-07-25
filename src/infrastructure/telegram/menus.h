#pragma once
#include "nlohmann/json.hpp"
#include <string>

namespace inline_menu{
    const std::string c_steam_menu_cb_data = "steam_main_menu";

    //Purchased list callback_data
    const std::string c_steam_purchase_list_menu_cb_data             = "steam_purchased_list_menu";
    const std::string c_steam_watch_list_menu_cb_data                = "steam_watch_list_menu";
    const std::string c_steam_survey_list_menu_cb_data               = "steam_survey_list_menu";
    const std::string c_steam_notification_list_menu_cb_data         = "steam_notification_list_menu";
    
    //Watch list callback_data
    const std::string c_steam_watch_list_prefix                 = "steam_watch_list";
    const std::string c_steam_watch_list_list_cb_data           = "steam_watch_list_list_btn";
    const std::string c_steam_watch_list_add_cb_data            = "steam_watch_list_add_btn";
    const std::string c_steam_watch_list_delete_list_cb_data    = "steam_watch_list_delete_list_btn";
    const std::string c_steam_watch_list_delete_cb_data         = "steam_watch_list_delete_btn";
    const std::string c_steam_watch_list_info_cb_data           = "steam_watch_list_info_btn";

    const std::string c_watch_list_delete_prefix = "wl_delete";

    //Survey list callback_data
    const std::string c_steam_survey_list_list_cb_data    = "steam_survey_list_list_btn";
    const std::string c_steam_survey_list_add_cb_data     = "steam_survey_list_add_btn";
    const std::string c_steam_survey_list_delete_cb_data  = "steam_survey_list_delete_btn";
    const std::string c_steam_survey_list_info_cb_data    = "steam_survey_list_info_btn";
    
    //Notification list callback_data
    const std::string c_steam_notification_list_list_cb_data    = "steam_notific_list_list_btn";
    const std::string c_steam_notification_list_add_cb_data     = "steam_notific_list_add_btn";
    const std::string c_steam_notification_list_delete_cb_data  = "steam_notific_list_delete_btn";
    const std::string c_steam_notification_list_info_cb_data    = "steam_notific_list_info_btn";
    
    //Purchased list callback_data
    const std::string c_steam_purchase_list_prefix          = "steam_purchase_list";
    const std::string c_steam_purchase_list_list_cb_data    = "steam_purchase_list_list_btn";
    const std::string c_steam_purchase_list_add_cb_data     = "steam_purchase_list_add_btn";
    const std::string c_steam_purchase_list_delete_cb_data  = "steam_purchase_list_delete_btn";
    const std::string c_steam_purchase_list_info_cb_data    = "steam_purchase_list_info_btn";

    const std::string c_purchase_list_delete_prefix = "pl_delete";

    class BotInlineMenus{
        using json = nlohmann::json;
    public:
        static std::vector<std::pair<std::string, std::string>> getMainMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("🎮Steam список", c_steam_menu_cb_data);
            return buttons;
        }

        static std::vector<std::pair<std::string, std::string>> getSteamMainMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("🛒Список покупок",        c_steam_purchase_list_menu_cb_data);
            buttons.emplace_back("👀Список отслеживания",   c_steam_watch_list_menu_cb_data);
            buttons.emplace_back("⏱️Список опросов",        c_steam_survey_list_menu_cb_data);
            buttons.emplace_back("⏰Список уведомлений",    c_steam_notification_list_menu_cb_data);
            buttons.emplace_back("🔄В главное меню",        c_steam_menu_cb_data);
            return buttons;
        }

        static std::vector<std::pair<std::string, std::string>> getSteamPurchasedMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("📋Отобразить список",  c_steam_purchase_list_list_cb_data);
            buttons.emplace_back("➕Добавить предмет",   c_steam_purchase_list_add_cb_data);
            buttons.emplace_back("➖Удалить предмет",    c_steam_purchase_list_delete_cb_data);
            buttons.emplace_back("📈Графики предметов",  c_steam_purchase_list_info_cb_data);
            buttons.emplace_back("🔄В главное меню",     c_steam_menu_cb_data);
            return buttons;
        }

        static std::vector<std::pair<std::string, std::string>> getSteamWatchMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("📋Отобразить список",  c_steam_watch_list_list_cb_data);
            buttons.emplace_back("➕Добавить предмет",   c_steam_watch_list_add_cb_data);
            buttons.emplace_back("➖Удалить предмет",    c_steam_watch_list_delete_list_cb_data);
            buttons.emplace_back("📈Графики предметов",  c_steam_watch_list_info_cb_data);
            buttons.emplace_back("🔄В главное меню",     c_steam_menu_cb_data);
            return buttons;
        }

        static std::vector<std::pair<std::string, std::string>> getSteamSurveyMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("📋Отобразить список",  c_steam_survey_list_list_cb_data);
            buttons.emplace_back("➕Добавить предмет",   c_steam_survey_list_add_cb_data);
            buttons.emplace_back("➖Удалить предмет",    c_steam_survey_list_delete_cb_data);
            buttons.emplace_back("📈Графики предметов",  c_steam_survey_list_info_cb_data);
            buttons.emplace_back("🔄В главное меню",     c_steam_menu_cb_data);
            return buttons;
        }
        static std::vector<std::pair<std::string, std::string>> getSteamNotificationMenuButtons(){
            std::vector<std::pair<std::string, std::string>> buttons;
            buttons.emplace_back("📋Отобразить список",  c_steam_notification_list_list_cb_data);
            buttons.emplace_back("➕Добавить предмет",   c_steam_notification_list_add_cb_data);
            buttons.emplace_back("➖Удалить предмет",    c_steam_notification_list_delete_cb_data);
            buttons.emplace_back("📈Графики предметов",  c_steam_notification_list_info_cb_data);
            buttons.emplace_back("🔄В главное меню",     c_steam_menu_cb_data);
            return buttons;
        }
        static json createInlineKeyboard(const std::vector<std::pair<std::string, std::string>>& buttons,
                                        const std::string& callback_prefix = "",
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
                    
                    
                    row.push_back(nlohmann::json{
                        {"text", text},
                        {"callback_data", callback_data}
                    });
                }
                
                keyboard_rows.push_back(row);
            }
            
            return keyboard;
        }
    };
}
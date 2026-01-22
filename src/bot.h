#pragma once
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "db.h"
#include "bot_context.h"
#include "price_overview_parser.h"
#include "price_history_parser.h"
#include "gnuplot_chart.h"
#include "user_context.h"
#include "worker_pool.h"

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
    ,m_workers_pool(std::thread::hardware_concurrency())
    {  initRequestsTable();  }
    
    void loop();
private:
    void initRequestsTable();
    void handleText(uint64_t chat_id, const std::string& text);
    json callRequest(TgAPIRequest request, const json& params);
    json callMethod(const std::string& method, RequestType type, const json& params);
    
    json getUpdates(uint64_t offset);
    void handleUpdate(const json& update);
    bool sendMessage(uint64_t chat_id, const std::string& text, const json& inline_keyboard, ParseMode mode, bool disable_web_preview, const std::string& espace_symbols);
    void getBotname();
    
    void getMe();
    void parseGetMeResponse(json response);

    bool forwardMessages(uint64_t chat_id, uint64_t from_chat_id, const std::vector<uint64_t>& message_ids);
    bool forwardMessage(uint64_t chat_id, uint64_t from_chat_id, uint64_t message_id);

    json getMyCommands();
    json getFile(const std::string& file_id);
    
    bool downloadTelegramFile(const std::string& file_path, const std::string& save_as);
    bool uploadTelegramPhoto(const std::string file_path);
    std::string sendLocalFile(const std::string& token, long long chat_id, const std::string& file_path, const std::string& caption, const std::string& method);
    std::string sendPhotoFile(long long chat_id,  const std::string& file_path, const std::string& caption);

    bool setChatMenuButton(uint64_t chat_id);
    json getAvailableGifts();

    //InlineKeyboard
    json mainMenu();
    json steamMenu();
    json steamWatchListMenu();
    json steamPurchaseListMenu();
    json steamWatchListAddLinkMenu();
    
    nlohmann::json createInlineKeyboard(const std::vector<std::string>& buttons, const std::string& callback_prefix, int rows);

    void handleCallbackQuery(const json& callback);
    
    //DataBase
    void initDatabase();
    bool addSteamLink(uint64_t chat_id, const std::string& line);
    bool deleteSteamLink(uint64_t, const std::string& title);

    json getUserLinkPriceOverview(const json& link);
    std::string convertUserLinkMinimal(const json& link);

    std::string getUserItemChart(const json& link);
    std::string getUserItemPriceAnalysys(const json& link, const json& price);
private:
    std::string m_token;
    std::string m_name;
    uint64_t    m_id;
    bool        m_authorized;
    std::unique_ptr<DataBase> m_sqlite_db;
    BotContext m_context;
    WorkerPool m_workers_pool;
    
    std::unordered_map<TgAPIRequest, std::tuple<std::string, RequestType>> m_requests_table;
    std::unordered_map<uint64_t, UserContext::ItemBuyInfo> m_user_buy_item_info;

    // const size_t c_workers_threads = ;

    const std::string c_main_menu_string                  = "main_menu";
    const std::string c_steam_menu_string                 = "steam_menu";
    const std::string c_steam_purchase_list_menu_string   = "steam_purchased_list_menu";
    const std::string c_steam_watch_list_menu_string      = "steam_watch_list_menu";

    const std::string c_steam_add_watch_list_string                = "steam_wl_add";
    const std::string c_steam_delete_watch_list_string             = "steam_wl_delete";
    const std::string c_steam_list_watch_list_string               = "steam_wl_list";
    const std::string c_steam_info_watch_list_string               = "steam_wl_info";

    const std::string c_steam_add_purchased_item_string            = "steam_pl_add_buy_info";
    const std::string c_steam_delete_purchased_item_string         = "steam_pl_delete_buy_info";
    const std::string c_steam_purchased_item_info_string           = "steam_pl_items_buy_info";
    const std::string c_steam_list_purchased_items_string          = "steam_pl_list_items_purchased";
    
    const std::string c_steam_app_id        = "730"; //CS2 app id

    const PriceOverview::SteamCurrency c_steam_currency = PriceOverview::SteamCurrency::eUSD; //Current steam currency
    
};

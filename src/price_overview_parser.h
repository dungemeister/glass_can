#pragma once
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include <string>
#include <sstream>

namespace PriceOverview{

    struct SteamItem {
        bool success = false;
        std::string lowest_price;
        std::string median_price;
        std::string volume;
    };

    enum SteamCurrency{
        eUSD = 1,
        eEUR = 3,
        eRUB = 5,
    };
    struct Parser{


        static std::string getSteamItemPrice(const std::string& appid, const std::string& market_hash_name, SteamCurrency currency=SteamCurrency::eUSD){
            try {
                curlpp::Cleanup cleanup;
                curlpp::Easy request;
                
                std::string url = "https://steamcommunity.com/market/priceoverview/?";
                url += "appid=" + appid;
                url += "&currency=" + std::to_string(currency);
                url += "&market_hash_name=" + market_hash_name;
                
                std::stringstream response;
                
                request.setOpt(new curlpp::options::Url(url));
                request.setOpt(new curlpp::options::UserAgent(
                    "Mozilla/5.0 (X11; Linux x86_64; rv:141.0) Gecko/20100101 Firefox/141.0"));
                
                // Правильный способ для curlpp 0.8.1
                request.setOpt(new curlpp::options::WriteStream(&response));
                
                request.setOpt(new curlpp::options::FollowLocation(true));
                request.setOpt(new curlpp::options::Timeout(30));
                
                request.perform();
                return response.str();
                
            } catch (const curlpp::LibcurlRuntimeError& e) {
                return "{\"success\":false,\"error\":\"curl: " + std::string(e.what()) + "\"}";
            } catch (const curlpp::LogicError& e) {
                return "{\"success\":false,\"error\":\"logic: " + std::string(e.what()) + "\"}";
            }
        }
    };
} //namespace PriceOverview

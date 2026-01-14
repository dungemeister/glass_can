#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <nlohmann/json.hpp>

namespace PriceHistory{
    struct PricePoint {
        std::string date;   // как пришло от Steam
        double median = 0;  // цена
        int volume = 0;     // объем
    };

    enum eTimePeriod{
        WEEK,
        MONTH,
        YEAR,
        ALL_TIME
    };
    static std::string getTimePeriodString(eTimePeriod period){
        switch(period){
            case eTimePeriod::WEEK:
                return "week";
            case eTimePeriod::MONTH:
                return "month";
            case eTimePeriod::YEAR:
                return "year";
            case eTimePeriod::ALL_TIME:
                return "all_time";
        }
        return "unknown";
    }

    static std::string getCookieFromFile(const std::string& filepath){
        std::string web_cookie;
        std::fstream cookie_file(filepath);
        if(!cookie_file.is_open()){
            std::cerr <<"Fail to open " << filepath << std::endl;
            return {};
        }
        while(std::getline(cookie_file, web_cookie)){}
        
        return web_cookie;
    }

    static std::vector<PricePoint> parsePriceHistoryJson(const std::string& body, eTimePeriod period) {
        nlohmann::json j = nlohmann::json::parse(body);
        std::vector<PricePoint> out;
        auto& prices = j["prices"];

        if (!j.value("success", false)) return out;

        size_t points_count = 0;
        switch(period){
            case eTimePeriod::ALL_TIME:
                points_count = 0;
            break;
            case eTimePeriod::WEEK:
                points_count = 24 * 7;
            break;
            case eTimePeriod::MONTH:
                points_count = 24 * 30;
            break;
            case eTimePeriod::YEAR:
                points_count = 23 * 365;
            break;
        }
        points_count = std::clamp<size_t>(points_count, 0, prices.size());
        auto rit_end = points_count > 0 ? prices.rbegin() + points_count: prices.rend();

        for(auto rit = prices.rbegin(); rit != rit_end; ++rit ){
            
            PricePoint pt;
            pt.date   = rit->at(0).get<std::string>();
            pt.median = rit->at(1).get<double>();
            pt.volume = std::stoi(rit->at(2).get<std::string>());
            out.push_back(std::move(pt));
        }

        std::reverse(out.begin(), out.end());
        // for (const auto& p : j["prices"]) {
        //     // p = [ "Nov 27 2013 01: +0", 12.767, "1" ] [web:161]
        //     PricePoint pt;
        //     pt.date   = p.at(0).get<std::string>();
        //     pt.median = p.at(1).get<double>();
        //     pt.volume = std::stoi(p.at(2).get<std::string>());
        //     out.push_back(std::move(pt));
        // }
        return out;
    }

    static std::string pricehistory(const std::string& market_hash_name) {
        std::string url = "https://steamcommunity.com/market/pricehistory/?"
            "appid=730&market_hash_name=" + market_hash_name + "&currency=1";
        
        getCookieFromFile("web.cookie");
        std::string cookies = "steamLoginSecure=76561198064492302||eyAidHlwIjogIkpXVCIsICJhbGciOiAiRWREU0EiIH0.eyAiaXNzIjogInI6MDAxN18yNzg3M0Q2OF9GQ0UxNiIsICJzdWIiOiAiNzY1NjExOTgwNjQ0OTIzMDIiLCAiYXVkIjogWyAid2ViOmNvbW11bml0eSIgXSwgImV4cCI6IDE3Njg0MDM2NzQsICJuYmYiOiAxNzU5Njc3MjA3LCAiaWF0IjogMTc2ODMxNzIwNywgImp0aSI6ICIwMDE5XzI3ODczRDc5XzRCRTE3IiwgIm9hdCI6IDE3NjgyMTgyMTUsICJydF9leHAiOiAxNzg2NDU2NjkzLCAicGVyIjogMCwgImlwX3N1YmplY3QiOiAiOTIuNTQuMjA0LjExNiIsICJpcF9jb25maXJtZXIiOiAiOTIuNTQuMjA0LjExNiIgfQ.qTmZqm9fmrvj3fgHGHmXLA347uuRMSnJErdeXUlSwPgfxDVg1Wp8ZDBOWy3OfqbyjijfamYMLOhHI8DxSimgAQ;"
                            "sessionid=d343d1419729e399cf66c063";

        curlpp::Easy req;
        req.setOpt(new curlpp::options::Url(url));
        
        // МИНИМУМ 2026:
        req.setOpt(new curlpp::options::Cookie(cookies));
        req.setOpt(new curlpp::options::Referer("https://steamcommunity.com/market/"));
        req.setOpt(new curlpp::options::UserAgent("Mozilla/5.0 (X11; Linux x86_64; rv:141.0) Gecko/20100101 Firefox/141.0"));
        
        std::stringstream response;
        req.setOpt(new curlpp::options::WriteStream(&response));
        req.perform();
        
        return response.str();
    }
    static void savePriceHistoryToCSV(const std::vector<PricePoint>& points, const std::string& filename) {
        std::ofstream csv(filename);
        
        // Заголовки
        csv << "Date,Price_USD,Volume,Price_Change_24h\n";
        
        if (!points.empty()) {
            double prevPrice = points[0].median;
            
            for (size_t i = 0; i < points.size(); ++i) {
                const auto& p = points[i];
                
                // Форматируем дату
                std::string date = p.date.substr(0, 16); // "Jan 12 2026 12:00"
                
                // Изменение цены
                double change24h = (i > 0) ? 
                    (p.median - prevPrice) / prevPrice * 100 : 0;
                prevPrice = p.median;
                
                // Запись CSV (с экранированием)
                csv << "\"" << date << "\"," 
                    << std::fixed << std::setprecision(4) << p.median << ","
                    << p.volume << "," 
                    << std::setprecision(2) << change24h << "%\n";
            }
        }
        
        csv.close();
        printf("✅ %s сохранён (откройте в Excel/Google Sheets)\n", filename.c_str());
    }

    static std::vector<PricePoint> getPriceHistory(const std::string& market_hash_name, eTimePeriod period = eTimePeriod::ALL_TIME){
        return parsePriceHistoryJson(pricehistory(market_hash_name), period);
    }


}; //namespace PriceHistory
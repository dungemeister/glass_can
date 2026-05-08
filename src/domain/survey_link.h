#pragma once
#include <string>

struct SurveyLink{
    SurveyLink(){

    }

    SurveyLink(const nlohmann::json& data){
        url = data["url"].get<std::string>();
        title = data["title"].get<std::string>();
        period = static_cast<decltype(period)>(std::atoi(data["period"].get<std::string>().c_str()));
        chat_id = static_cast<decltype(chat_id)>(std::atoi(data["user_id"].get<std::string>().c_str()));
    }

    SurveyLink(const std::string& _url, const std::string& _title, int _period, int _chat_id)
    :url(_url)
    ,title(_title)
    ,period(_period)
    ,chat_id(_chat_id)
    {}
    
    std::string url;
    std::string title;
    int period;
    int chat_id;

    std::string repr() const { return url + " " + title + " " + std::to_string(period); }
};
#pragma once
#include <string>
#include <functional>

/// @brief 
/// @warning std::hash is used to calculate hash and return a 'size_t' value that differs on different platforms
struct SurveyLink{
    SurveyLink(){

    }

    SurveyLink(const nlohmann::json& data){
        url = data["url"].get<std::string>();
        title = data["title"].get<std::string>();
        period = static_cast<decltype(period)>(std::atoi(data["period"].get<std::string>().c_str()));
        chat_id = static_cast<decltype(chat_id)>(std::atoi(data["user_id"].get<std::string>().c_str()));
        task_id_hash = static_cast<decltype(task_id_hash)>(std::atoi(data["task_id_hash"].get<std::string>().c_str()));
        
    }

    SurveyLink(const std::string& _url, const std::string& _title, int _period, int _chat_id)
    :url(_url)
    ,title(_title)
    ,period(_period)
    ,chat_id(_chat_id)
    {
        task_id_hash = calc_task_hash();
    }
    
    std::string url;
    std::string title;
    int period;
    int chat_id;
    size_t task_id_hash;

    std::string repr() const { return url + " " + title + " " + std::to_string(period); }

    size_t calc_task_hash() const { 
        std::hash<std::string> hasher;
        return hasher(repr());
    }
};
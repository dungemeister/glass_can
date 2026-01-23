#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <exception>
#include <nlohmann/json.hpp>

struct BotConfig{
    BotConfig(std::string config_file)
    :m_config_file(config_file)
    {
        m_bot_token = get_from_env(c_token_string.c_str());
        m_cookie_jar = get_from_env(c_cookie_string.c_str());

        parse_config_file();

    }
    std::string getBotToken()         const { return m_bot_token; }
    std::string getSteamSessionID()   const { return m_steam_session_id; }
    std::string getSteamLoginSecure() const { return m_steam_login_secure; }
    size_t      getBotWorkers()       const { return m_config_json["bot"]["workers"].get<size_t>(); }
    
private:
    void parse_config_file(){
        std::fstream file(m_config_file, std::ios::out | std::ios::in | std::ios::ate);
        if(!file.is_open()){
            throw std::runtime_error("Fail to open " + m_config_file);
        }
        std::string line;
        std::string data;
        data.reserve(static_cast<long>(file.tellg()));
        file.seekg(0, std::ios::beg);
        while(std::getline(file, line)){
            data += line;
        }
        m_config_json = nlohmann::json::parse(data);
        file.close();
    }
    std::string get_from_env(const char* key){
        auto value = std::getenv(key);

        if(!value || !*value) throw std::runtime_error(std::string("Missing env key ") + key);

        return value;
    }

private:
    std::string    m_config_file;
    nlohmann::json m_config_json;

    std::string m_cookie_jar;

    std::string m_bot_token;
    std::string m_steam_session_id;
    std::string m_steam_login_secure;

    const std::string c_token_string = "TGBOT_TOKEN";
    const std::string c_cookie_string = "COOKIE_JSON";
};
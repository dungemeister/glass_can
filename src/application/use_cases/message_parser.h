#pragma once

#include "tg_events.h"
#include "string_misc.h"
#include "nlohmann/json.hpp"

class MessageParser{
public:
    using json = nlohmann::json;

    BotEvent operator()(const json& msg){
        BotEvent event;
        json json_msg = {};
        json json_callback_msg = {};
        auto it = msg.find("callback_query");
        if(it == msg.end()){
            json_msg = msg["message"];
            auto text = json_msg["text"].get<std::string>();
            if (text[0] == '/'){
                event = CommandEvent();
            }
            else{

                event = MessageEvent();
            }
        }
        else {
            json_msg = it.value();
            event = CallbackEvent();
        }
        // std::cout << json_msg.dump(2) << std::endl;
        if(std::holds_alternative<CallbackEvent>(event)){
            auto& event_ = std::get<CallbackEvent>(event);
            event_.chat_id              = json_msg["from"]["id"].get<int>();
            event_.message_id           = json_msg["message"]["message_id"].get<int>();
            event_.username             = json_msg["from"]["username"].get<std::string>();
            event_.callback_data        = json_msg["data"].get<std::string>();
            event_.callback_query_id    = json_msg["id"].get<std::string>();

        }
        else if(std::holds_alternative<MessageEvent>(event)){
            auto& event_ = std::get<MessageEvent>(event);
            event_.chat_id      = json_msg["chat"]["id"].get<int>();
            event_.message_id   = json_msg["message_id"].get<int>();
            event_.username     = json_msg["chat"]["username"].get<std::string>();
            event_.text         = json_msg["text"].get<std::string>();
        }
        else if(std::holds_alternative<CommandEvent>(event)){
            auto & event_ = std::get<CommandEvent>(event);
            event_.chat_id      = json_msg["chat"]["id"].get<int>();
            event_.message_id   = json_msg["message_id"].get<int>();
            event_.username     = json_msg["chat"]["username"].get<std::string>();
            
            auto tokens = StringMisc::splitByDelim(json_msg["text"].get<std::string>(), ' ');
            event_.command      = tokens[0];
            if(tokens.size() > 1){
                event_.args.insert(event_.args.end(),
                             std::make_move_iterator(tokens.begin() + 1),
                             std::make_move_iterator(tokens.end()));
                tokens.clear();

            }
            else{
                event_.args = {};
            }

        }
        return event;
    }
};
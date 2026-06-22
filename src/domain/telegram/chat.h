#pragma once
#include <string>
#include <optional>

class TelegramChat{
public:
    TelegramChat(   int                         _id,
                    const std::string&          _type,
                    std::optional<std::string>  _title = std::nullopt,
                    std::optional<std::string>  _username = std::nullopt,
                    std::optional<std::string>  _first_name = std::nullopt,
                    std::optional<std::string>  _last_name = std::nullopt,
                    std::optional<bool>         _forum = std::nullopt,
                    std::optional<bool>         _direct_messages = std::nullopt)
    :id(_id)
    ,type(_type)
    ,title(_title)
    ,username(_username)
    ,first_name(_first_name)
    ,last_name(_last_name)
    ,forum(_forum)
    ,direct_messages(_direct_messages)
    {}

    std::string                 get_type() const            { return type; }
    std::optional<std::string>  get_title() const           { return title; }
    std::optional<std::string>  get_username() const        { return username; }
    std::optional<std::string>  get_first_name() const      { return first_name; }
    std::optional<std::string>  get_last_name() const       { return last_name; }
    std::optional<bool>         is_forum() const            { return forum; }
    std::optional<bool>         is_direct_messages() const  { return direct_messages; }
private:
    int id;
    std::string type;
    std::optional<std::string> title;
    std::optional<std::string> username;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::optional<bool> forum;
    std::optional<bool> direct_messages;

};
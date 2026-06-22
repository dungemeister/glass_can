#pragma once
#include <optional>
#include <string>

class TgUser{
public:
    TgUser( int _id,
            bool _bot,
            const std::string& _first_name,
            const std::optional<std::string> _last_name,
            const std::optional<std::string> _username,
            const std::optional<std::string> _language_group
            )
    :id(_id)
    ,bot(_bot)
    ,first_name(_first_name)
    ,username(_username)
    ,language_group(_language_group)
    {} 

    int                         get_id() const              { return id; }
    bool                        is_bot() const              { return bot; }
    std::string                 get_first_name() const      { return first_name; }
    std::optional<std::string>  get_last_name() const       { return last_name; }
    std::optional<std::string>  get_username() const        { return username; }
    std::optional<std::string>  get_language_group() const  { return language_group; }

    bool operator==(const TgUser& other) const {
        return (id == other.id &&
                bot == other.bot &&
                first_name == other.first_name &&
                username == other.username);
                
    }
private:    
    int id;
    bool bot;
    std::string first_name;
    std::optional<std::string> last_name;
    std::optional<std::string> username;
    std::optional<std::string> language_group;
    std::optional<bool> is_premium;
    std::optional<bool> added_to_attachment_menu;
    std::optional<bool> can_join_groups;
    std::optional<bool> can_read_all_group_messages;
    std::optional<bool> support_guest_queries;
    std::optional<bool> support_inline_queries;
    std::optional<bool> can_connect_bussiness;
    std::optional<bool> has_main_web_app;
    std::optional<bool> has_topics_enabled;
    std::optional<bool> allows_users_to_create_topics;
    std::optional<bool> can_manage_bots;

};
#pragma once
#include "user.h"
#include "db_pagination.h"
#include <vector>
#include <optional>

class IUserRepository{
public:
    virtual ~IUserRepository() = default;
    
    virtual std::vector<User> getUsers(const DataBasePagination& pag) = 0;
    virtual std::optional<User> getUser(int chat_id) = 0;

    virtual void save(const User& user) = 0;
    
};

#pragma once
#include "user_irepository.h"

class UserService{
public:
    UserService(IUserRepository& rep)
    :m_rep(rep){}

    std::optional<User> getUserFromChatId(int chat_id) const {
        return m_rep.getUser(chat_id);
    }
    
    void addUser(const User& user){
        return m_rep.save(user);
    }

    std::vector<User> getUsers(const DataBasePagination& pag) const{
        return m_rep.getUsers(pag);
    }

    void softDeleteUser(User& user) {
        //TODO: Refactor db and bussiness logic to deactivate user
        user.deactivate();
        m_rep.save(user);
    }
private:
    IUserRepository& m_rep;
};
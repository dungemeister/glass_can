#pragma once
#include "link_repository.h"
#include "watch_link.h"

class WatchLinkService{
public:
    WatchLinkService(ILinkRepository& rep)
    :m_rep(rep) {}

    ~WatchLinkService() = default;

    std::vector<WatchLink> getLinks() const{

    }
    void addLink(const WatchLink& link);
    void deleteLink(const WatchLink& link);
private:
    ILinkRepository& m_rep;
};
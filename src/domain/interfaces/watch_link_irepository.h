#pragma once
#include "user.h"
#include "watch_link.h"
#include <vector>

class IWatchLinkRepository{
public:
    virtual ~IWatchLinkRepository() = default;

    virtual std::vector<WatchLink>  getLinks(const User& user) = 0;
    virtual void                    addLink(const WatchLink& link) = 0;
    virtual void                    deleteLink(const WatchLink& link) = 0;

};
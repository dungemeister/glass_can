#pragma once
#include "user.h"
#include "user_service.h"

class GlassCanApp{
public:
    GlassCanApp();
    ~GlassCanApp() = default;

    void serve();
private:
    void loop();
    UserService m_user_service;
};
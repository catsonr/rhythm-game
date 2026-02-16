#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class AuthCtrl : public drogon::HttpController<AuthCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthCtrl::login, "/login", drogon::Post);
        ADD_METHOD_TO(AuthCtrl::logout, "/logout", drogon::Post);
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
    }

    void logout(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
    }
}; // AuthCtrl
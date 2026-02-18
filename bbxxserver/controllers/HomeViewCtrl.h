#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class HomeViewCtrl : public drogon::HttpController<HomeViewCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(HomeViewCtrl::serve, "/", drogon::Get, "WebsiteFilter");
    METHOD_LIST_END

    void serve(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        std::string host = req->getHeader("host");

        drogon::HttpViewData view_data;
        view_data.insert("host", host);

        callback( drogon::HttpResponse::newHttpViewResponse("home", view_data) );
    }

}; // HomeViewCtrl
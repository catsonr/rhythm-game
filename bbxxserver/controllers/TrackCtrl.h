#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class TrackCtrl : public drogon::HttpController<TrackCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TrackCtrl::get, "/tracks/{id}", drogon::Get);
        ADD_METHOD_TO(TrackCtrl::get_all, "/tracks", drogon::Get);
        ADD_METHOD_TO(TrackCtrl::create, "/tracks", drogon::Post);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {}

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {}

    void create(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {}
}; // TrackCtrl
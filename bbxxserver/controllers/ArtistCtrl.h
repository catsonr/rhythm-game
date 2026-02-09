#pragma once

#include <drogon/HttpController.h>

class ArtistCtrl : public drogon::HttpController<ArtistCtrl>
{
  public:
    METHOD_LIST_BEGIN
      ADD_METHOD_TO(ArtistCtrl::get, "/artists/{id}", drogon::Get);
      ADD_METHOD_TO(ArtistCtrl::get_all, "/artists", drogon::Get);
      ADD_METHOD_TO(ArtistCtrl::create, "/artists", drogon::Post);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback, int id);
    void get_all(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback);
    void create(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback);
};

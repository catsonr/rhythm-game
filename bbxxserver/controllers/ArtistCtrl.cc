#include "ArtistCtrl.h"

#include "../models/storage.h"

void ArtistCtrl::get(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback, int id)
{
    auto storage = server::init_storage();
    
    try
    {
        const server::Artist& artist = storage.get<server::Artist>(id);
        
        Json::Value ret;
        ret["id"] = artist.id;
        ret["name"] = artist.name;

        callback(drogon::HttpResponse::newHttpJsonResponse(ret));
    }
    catch(const std::exception& e)
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("artist with id (" + std::to_string(id) + ") not found!");

        callback(resp);
    }
    
}

void ArtistCtrl::get_all(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback)
{
    auto storage = server::init_storage();
    std::vector<server::Artist> artists = storage.get_all<server::Artist>();
    
    Json::Value ret;
    Json::Value list(Json::arrayValue);
    
    for(const server::Artist& artist : artists)
    {
        Json::Value item;
        item["id"] = artist.id;
        item["name"] = artist.name;

        list.append(item);
    }
    
    ret["artists"] = list;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);

    callback(resp);
}

void ArtistCtrl::create(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback)
{
    std::shared_ptr<Json::Value> json_ptr = req->getJsonObject();
    
    if( !json_ptr )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("could not parse JSON!");
        
        callback(resp);
        return;
    }
    if( !json_ptr->isMember("name") )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("missing required field 'name'!");
        
        callback(resp);
        return;
    }
    
    server::Artist artist;
    artist.name = (*json_ptr)["name"].asString();
    
    try
    {
        auto storage = server::init_storage();
        int id = storage.insert(artist);
        
        Json::Value ret;
        ret["id"] = id;
        ret["name"] = artist.name;

        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k201Created);

        callback(resp);
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "db error: " << e.what();
        
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("failed to add new arist to db!");
        
        callback(resp);
    }
    
}

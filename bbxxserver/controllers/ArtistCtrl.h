#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class ArtistCtrl : public drogon::HttpController<ArtistCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ArtistCtrl::get, "/artists/{id}", drogon::Get);
        ADD_METHOD_TO(ArtistCtrl::get_all, "/artists", drogon::Get);
        ADD_METHOD_TO(ArtistCtrl::create, "/artists", drogon::Post);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();

        try
        {
            const server::Artist &artist = storage.get<server::Artist>(id);

            Json::Value ret;
            ret["id"] = artist.id;
            ret["name"] = artist.name;

            callback(drogon::HttpResponse::newHttpJsonResponse(ret));
        }
        catch (const std::exception &e)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("artist with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }
    }

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        server::Storage storage = server::init_storage();
        std::vector<server::Artist> artists = storage.get_all<server::Artist>();
        
        Json::Value artists_json { Json::arrayValue };
        
        for(const server::Artist& artist : artists)
        {
            Json::Value item;
            item["id"] = artist.id;
            item["name"] = artist.name;

            artists_json.append(item);
        }
        
        Json::Value json;
        json["artists"] = artists_json;
        
        callback( drogon::HttpResponse::newHttpJsonResponse(json) );
    }

    void create(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
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
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k201Created, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("new artist '" + artist.name + "' created :)");

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
}; // ArtistCtrl

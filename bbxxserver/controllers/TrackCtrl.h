#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class TrackCtrl : public drogon::HttpController<TrackCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TrackCtrl::get, "/tracks/{id}", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::get_all, "/tracks", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::create, "/tracks", drogon::Post, "ApiFilter");
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();
        
        std::optional<server::Track> track = storage.get_optional<server::Track>(id);
        
        if(!track)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("track with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }
        
        Json::Value json;
        json["id"] = track->id;
        json["title"] = track->title;
        json["ALBUM_ID"] = track->ALBUM_ID;

        callback(drogon::HttpResponse::newHttpJsonResponse(json));
    }

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        server::Storage storage = server::init_storage();
        std::vector<server::Track> tracks = storage.get_all<server::Track>();
        
        Json::Value tracks_json { Json::arrayValue };
        for(const server::Track& track : tracks)
        {
            Json::Value item;
            item["id"] = track.id;
            item["title"] = track.title;
            item["ALBUM_ID"] = track.ALBUM_ID;
            
            tracks_json.append(item);
        }
        
        Json::Value json;
        json["tracks"] = tracks_json;
        
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
        if( !json_ptr->isMember("title") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'title'!");
            
            callback(resp);
            return;
        }
        if( !json_ptr->isMember("ALBUM_ID") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'ALBUM_ID'!");
            
            callback(resp);
            return;
        }
        
        server::Track track;
        track.title = (*json_ptr)["title"].asString();
        track.ALBUM_ID = (*json_ptr)["ALBUM_ID"].asUInt64();

        try
        {
            server::Storage storage = server::init_storage();
            uint64_t id = storage.insert(track);
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k201Created, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("new track '" + track.title + "' created :)");
            
            callback(resp);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "db error: " << e.what();
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to add new track to db!");
            
            callback(resp);
        }
    }
}; // TrackCtrl
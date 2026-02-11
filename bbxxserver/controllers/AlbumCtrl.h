#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class AlbumCtrl : public drogon::HttpController<AlbumCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AlbumCtrl::get, "/albums/{id}", drogon::Get);
        ADD_METHOD_TO(AlbumCtrl::get_all, "/albums", drogon::Get);
        ADD_METHOD_TO(AlbumCtrl::create, "/albums", drogon::Post);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();

        std::optional<server::Album> album = storage.get_optional<server::Album>(id);

        if (!album)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("album with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }

        Json::Value json;
        json["id"] = album->id;
        json["type"] = album->type;
        json["title"] = album->title;
        json["release_year"] = server::value_or_null(album->release_year);
        json["release_month"] = server::value_or_null(album->release_month);
        json["release_day"] = server::value_or_null(album->release_day);
        json["ARTIST_ID"] = album->ARTIST_ID;

        callback(drogon::HttpResponse::newHttpJsonResponse(json));
    }

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        server::Storage storage = server::init_storage();
        std::vector<server::Album> albums = storage.get_all<server::Album>();
        
        Json::Value albums_json { Json::arrayValue };
        
        for(const server::Album& album : albums)
        {
            Json::Value item;
            item["id"] = album.id;
            item["type"] = album.type;
            item["title"] = album.title;
            item["release_year"] = server::value_or_null(album.release_year);
            item["release_month"] = server::value_or_null(album.release_month);
            item["release_day"] = server::value_or_null(album.release_day);
            item["ARTIST_ID"] = album.ARTIST_ID;
            
            albums_json.append(item);
        }
        
        Json::Value json;
        json["albums"] = albums_json;

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
        if( !json_ptr->isMember("type") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'type'!");
            
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
        if( !json_ptr->isMember("ARTIST_ID") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'ARTIST_ID'!");
            
            callback(resp);
            return;
        }
        
        server::Album album;
        album.type = static_cast<server::Album::Type>( (*json_ptr)["type"].asUInt() );
        album.title = (*json_ptr)["title"].asString();
        // relese date values?
        album.ARTIST_ID = (*json_ptr)["ARTIST_ID"].asUInt64();
        
        try
        {
            server::Storage storage = server::init_storage();
            uint64_t id = storage.insert(album);
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k201Created, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("new album '" + album.title + "' created :)");
            
            callback(resp);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "db error: " << e.what();
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to add new album to db!");
            
            callback(resp);
        }
        
    }
}; // AlbumCtrl

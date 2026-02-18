#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class UserCtrl : public drogon::HttpController<UserCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(UserCtrl::get, "/users/{id}", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(UserCtrl::get_all, "/users", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(UserCtrl::create, "/users", drogon::Post, "ApiFilter");
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();
        
        std::optional<server::User> user = storage.get_optional<server::User>(id);
        
        if(!user)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("user with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }
        
        Json::Value json;
        json["id"] = user->id;
        json["username"] = user->username;
        // email
        json["password_hash"] = user->password_hash;

        callback(drogon::HttpResponse::newHttpJsonResponse(json));
    }

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        server::Storage storage = server::init_storage();
        std::vector<server::User> users = storage.get_all<server::User>();
        
        Json::Value users_json { Json::arrayValue };
        for(const server::User& user : users)
        {
            Json::Value item;
            item["id"] = user.id;
            item["username"] = user.username;
            // email
            item["password_hash"] = user.password_hash;
            
            users_json.append(item);
        }
        
        Json::Value json;
        json["users"] = users_json;
        
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
        if( !json_ptr->isMember("username") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'username'!");
            
            callback(resp);
            return;
        }
        if( !json_ptr->isMember("password_hash") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'password_hash'!");
            
            callback(resp);
            return;
        }
        
        server::User user;
        user.username = (*json_ptr)["username"].asString();
        // email
        user.password_hash = (*json_ptr)["password_hash"].asString();

        try
        {
            server::Storage storage = server::init_storage();
            uint64_t id = storage.insert(user);
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k201Created, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("new user '" + user.username + "' created :)");
            
            callback(resp);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "db error: " << e.what();
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to add new user to db!");
            
            callback(resp);
        }
    }
}; // UserCtrl
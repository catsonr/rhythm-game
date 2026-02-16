#pragma once

#include <drogon/HttpController.h>
#include <bcrypt/BCrypt.hpp>

#include "../models/storage.h"

class AuthCtrl : public drogon::HttpController<AuthCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthCtrl::registerr, "/register", drogon::Post);
        ADD_METHOD_TO(AuthCtrl::login, "/login", drogon::Post);
        ADD_METHOD_TO(AuthCtrl::logout, "/logout", drogon::Post);
    METHOD_LIST_END

    void registerr(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
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
        if( !json_ptr->isMember("password") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'password'!");
            
            callback(resp);
            return;
        }
        
        std::string username = (*json_ptr)["username"].asString();
        std::string password = (*json_ptr)["password"].asString();

        server::Storage storage = server::init_storage();
        std::vector<server::User> users = storage.get_all<server::User>(
            sqlite_orm::where(sqlite_orm::c(&server::User::username) == username)
        );
        
        if( users.size() != 0 )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("username '" + username + "' already taken!");
            
            callback(resp);
            return;
        }
        
        server::User user;
        user.username = username;
        user.password_hash = BCrypt::generateHash(password);

        try
        {
            int id = storage.insert(user);

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

    void login(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
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
        if( !json_ptr->isMember("password") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'password'!");
            
            callback(resp);
            return;
        }
        
        std::string username = (*json_ptr)["username"].asString();
        std::string password = (*json_ptr)["password"].asString();
        
        server::Storage storage = server::init_storage();
        std::vector<server::User> users = storage.get_all<server::User>(
            sqlite_orm::where(sqlite_orm::c(&server::User::username) == username)
        );
        
        if( users.size() != 1 )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("invalid username/password!");

            callback(resp);
            return;
        }
        
        const server::User& user = users[0];
        
        if( !BCrypt::validatePassword(password, user.password_hash) )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("invalid username/password!");

            callback(resp);
            return;
        }
        
        server::UserSession user_session;
        user_session.uuid = drogon::utils::getUuid();
        user_session.USER_ID = user.id;
        user_session.expire_time = trantor::Date::now().after(60 * 60 * 24).secondsSinceEpoch(); // valid for 24 hrs
        
        try
        {
            storage.replace(user_session); // we dont care about id; user_session primary key is uuid (so we need replace instead of insert)

            Json::Value json;
            json["uuid"] = user_session.uuid;

            callback(drogon::HttpResponse::newHttpJsonResponse(json));
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "db error: " << e.what();
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to add new user_session to db!");
            
            callback(resp);
        }
    }

    void logout(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
    }
}; // AuthCtrl
#pragma once

/*
    checks for HTTP header:
    "Authorization: Bearer uuid"
    where uuid is a UserSession uuid
*/

#include <drogon/HttpFilter.h>

#include "../models/storage.h"

struct AuthFilter : public drogon::HttpFilter<AuthFilter>
{
    AuthFilter() {}
    void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb)
    {
        std::string auth_header = req->getHeader("Authorization");
        
        if(auth_header.empty())
        {
            drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_TEXT_HTML);
            res->setBody("could not find Authorization header!");
            fcb(res);
            return;
        }
        
        if(auth_header.substr(0, 7) == "Bearer ")
        {
            std::string uuid = auth_header.substr(7);
            
            server::Storage storage = server::init_storage();
            std::unique_ptr<server::UserSession> user_session_ptr = storage.get_pointer<server::UserSession>(uuid);
            
            if( !user_session_ptr )
            {
                drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_TEXT_HTML);
                res->setBody("invalid uuid!");
                fcb(res);
                return;
            }
            if( user_session_ptr->expired(trantor::Date::now().secondsSinceEpoch()) )
            {
                drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k403Forbidden, drogon::ContentType::CT_TEXT_HTML);
                res->setBody("uuid is expired!");
                fcb(res);
                return;
            }
            
            req->attributes()->insert("USER_ID", user_session_ptr->USER_ID);

            // passed!
            fccb();
            return;
        }

        drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_TEXT_HTML);
        res->setBody("Authorization header type not accepted!");
        fcb(res);
        return;
    }
}; // AuthFilter
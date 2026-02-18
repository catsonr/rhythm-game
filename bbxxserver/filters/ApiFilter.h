#pragma once

/*
    ensures that there is the 'api' subdomain, e.g.:
    api.beatboxx.org or
    api.localhost:3939
    (as opposed to beatboxx.org or localhost:3939)
*/

#include <drogon/HttpFilter.h>

struct ApiFilter : public drogon::HttpFilter<ApiFilter>
{
    ApiFilter() {}
    void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb)
    {
        std::string host = req->getHeader("host");
        
        if( host == "api.beatboxx.org" || host == "api.localhost:3939" )
        {
            fccb();
            return;
        }

        drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_TEXT_HTML);
        res->setBody("please use the 'api' sub domain for this route k thanks");
        fcb(res);
        return;
    }
}; // ApiFilter
#pragma once

/*
    ensures that there is no subdomain, e.g.:
    beatboxx.org
    localhost:3939
    (as opposed to api.beatboxx.org or api.localhost:3939)
*/

#include <drogon/HttpFilter.h>

struct WebsiteFilter : public drogon::HttpFilter<WebsiteFilter>
{
    WebsiteFilter() {}
    void doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb)
    {
        std::string host = req->getHeader("host");
        
        if( host == "beatboxx.org" || host == "localhost:3939" )
        {
            fccb();
            return;
        }

        drogon::HttpResponsePtr res = drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_TEXT_HTML);
        res->setBody("please remove the sub domain for this route k thanks");
        fcb(res);
        return;
    }
}; // WebsiteFilter
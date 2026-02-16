#pragma once

#include <drogon/HttpController.h>

#include "../models/storage.h"

class ChartCtrl : public drogon::HttpController<ChartCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ChartCtrl::get, "/charts/{id}", drogon::Get);
        ADD_METHOD_TO(ChartCtrl::get_all, "/charts", drogon::Get);
        ADD_METHOD_TO(ChartCtrl::create, "/charts", drogon::Post);
    METHOD_LIST_END

    void get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();
        
        std::optional<server::Chart> chart = storage.get_optional<server::Chart>(id);
        
        if(!chart)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("chart with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }
        
        Json::Value json;
        json["id"] = chart->id;
        json["beats"] = server::vector_uint64_to_base64( chart->beats );
        json["notes"] = server::vector_uint64_to_base64( chart->notes );

        callback(drogon::HttpResponse::newHttpJsonResponse(json));
    }

    void get_all(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        server::Storage storage = server::init_storage();
        std::vector<server::Chart> charts = storage.get_all<server::Chart>();
        
        Json::Value charts_json { Json::arrayValue };
        for(const server::Chart& chart : charts)
        {
            Json::Value item;
            item["id"] = chart.id;
            item["beats"] = server::vector_uint64_to_base64( chart.beats );
            item["notes"] = server::vector_uint64_to_base64( chart.notes );
            
            charts_json.append(item);
        }
        
        Json::Value json;
        json["charts"] = charts_json;
        
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
        if( !json_ptr->isMember("TRACK_ID") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'TRACK_ID'!");
            
            callback(resp);
            return;
        }
        if( !json_ptr->isMember("USER_ID") )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("missing required field 'USER_ID'!");
            
            callback(resp);
            return;
        }
        
        server::Chart chart;
        chart.TRACK_ID = (*json_ptr)["TRACK_ID"].asUInt64();
        chart.USER_ID  = (*json_ptr)["USER_ID"].asUInt64();
        chart.beats = server::base64_to_vector_uint64( (*json_ptr)["beats"].asString() );
        chart.notes = server::base64_to_vector_uint64( (*json_ptr)["notes"].asString() );

        try
        {
            server::Storage storage = server::init_storage();
            uint64_t id = storage.insert(chart);
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k201Created, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("new chart created :)");
            
            callback(resp);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "db error: " << e.what();
            
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to add new chart to db!");
            
            callback(resp);
        }
    }
}; // ChartCtrl
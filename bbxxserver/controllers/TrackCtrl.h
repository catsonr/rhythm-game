#pragma once

#include <cstdio>
#include <sstream>
#include <unordered_set>

#include <drogon/HttpController.h>

#include "../models/storage.h"

class TrackCtrl : public drogon::HttpController<TrackCtrl>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TrackCtrl::get, "/tracks/{id}", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::get_all, "/tracks", drogon::Get, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::create, "/tracks", drogon::Post, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::fingerprint, "/tracks/{id}/fingerprint", drogon::Patch, "ApiFilter");
        ADD_METHOD_TO(TrackCtrl::find_fingerprint, "/tracks/fingerprint", drogon::Post, "ApiFilter");
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
        json["fingerprinted"] = track->fingerprinted();
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
            item["fingerprinted"] = track.fingerprinted();
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
    
    // generates the raw fingerprint of a drogon::HttpFile
    static std::optional<std::vector<uint32_t>> generate_raw_fingerprint(const drogon::HttpFile& file)
    {
        // save file to disk temporarily
        std::string uuid = drogon::utils::getUuid();
        std::string temp_file_name = uuid + ".";
        temp_file_name.append(file.getFileExtension());
        
        std::filesystem::path upload_dir = drogon::app().getUploadPath();
        std::filesystem::path full_path = upload_dir / temp_file_name;
        
        if( file.saveAs(temp_file_name) != 0 )
        {
            printf("[TrackCtrl::generate_raw_fingerprint] failed to save '%s'!", full_path.c_str());
            return std::nullopt;
        }

        // run fpcalc
        std::string command = "fpcalc -raw -plain \"" + full_path.string() + "\" 2>&1";

        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if( !pipe )
        {
            printf("[TrackCtrl::generate_raw_fingerprint] popen() failed!");
            return std::nullopt;
        }
        
        while( fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr ) result += buffer.data();
        if( result.substr(0, 5) == "ERROR" )
        {
            printf("[TrackCtrl::generate_raw_fingerprint] fpcalc gave an error:\n%s", result.c_str());
            return std::nullopt;
        }

        // remove file from disk
        if( std::filesystem::exists(full_path) ) std::filesystem::remove(full_path);
        
        // parse output into vector
        std::vector<uint32_t> raw_fingerprint;
        std::istringstream stream(result);
        std::string token;
        while( std::getline(stream, token, ',') )
        {
            raw_fingerprint.push_back( static_cast<uint32_t>(std::stoul(token)) );
        }
        
        return raw_fingerprint;
    }
    
    // returns the (single) file uploaded in form-data, and nullopt otherwise
    std::optional<drogon::HttpFile> get_file_from_req(const drogon::HttpRequestPtr& req)
    {
        drogon::MultiPartParser parser;
        if( parser.parse(req) != 0 )
        {
            printf("[TrackCtrl::get_file_from_req] failed to parse!");
            return std::nullopt;
        }
        
        std::vector<drogon::HttpFile> files = parser.getFiles();
        if( files.empty() )
        {
            printf("[TrackCtrl::get_file_from_req] no files uploaded!");
            return std::nullopt;
        }
        if( files.size() > 1 )
        {
            printf("[TrackCtrl::get_file_from_req] more than one file uploaded!");
            return std::nullopt;
        }
        
        return files[0];
    }

    void fingerprint(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, int id)
    {
        server::Storage storage = server::init_storage();

        // find track
        std::optional<server::Track> track = storage.get_optional<server::Track>(id);
        if(!track)
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k404NotFound, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("track with id (" + std::to_string(id) + ") not found!");

            callback(resp);
            return;
        }
        
        // find file
        std::optional<drogon::HttpFile> file = get_file_from_req(req);
        if( !file )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("could not parse form-data! please upload a single audio file");

            callback(resp);
            return;
        }
        
        // generate raw fingerprint!
        std::optional<std::vector<uint32_t>> raw_fingerprint = TrackCtrl::generate_raw_fingerprint(file.value());
        if( !raw_fingerprint )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::HttpStatusCode::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to fingerprint '" + track->title + "' ... (did you upload the right file?)");

            callback(resp);
            return;
        }

        // update db
        track->raw_fingerprint = raw_fingerprint.value();
        storage.update(*track);

        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::HttpStatusCode::k200OK, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("'" + track->title + "' fingerprinted!");

        callback(resp);
        return;
    }

    // returns the number shared hashes between two raw fingerprints
    static int compare_fingerprints(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b)
    {
        if( a.empty() || b.empty() ) return 0;

        std::unordered_set<uint32_t> a_set(a.begin(), a.end());
        
        int matches = 0;
        for( uint32_t hash : b ) if( a_set.contains(hash) ) matches++;
        
        return matches;
    }

    void find_fingerprint(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback)
    {
        // find file
        std::optional<drogon::HttpFile> file = get_file_from_req(req);
        if( !file )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("could not parse form-data! please upload a single audio file");

            callback(resp);
            return;
        }

        // generate raw fingerprint from file
        std::optional<std::vector<uint32_t>> file_raw_fingerprint = generate_raw_fingerprint(file.value());
        if( !file_raw_fingerprint )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::HttpStatusCode::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("failed to fingerprint '" + file->getFileName() + "' ... (did you upload the right file?)");

            callback(resp);
            return;
        }

        // grab all tracks and compare fingerprints
        server::Storage storage = server::init_storage();
        std::vector<server::Track> tracks = storage.get_all<server::Track>();

        Json::Value results(Json::arrayValue);
        int file_hash_count = file_raw_fingerprint->size();

        for( const server::Track& track : tracks )
        {
            if( !track.fingerprinted() ) continue;

            int matches = compare_fingerprints(track.raw_fingerprint.value(), file_raw_fingerprint.value());
            double confidence = static_cast<double>(matches) / file_hash_count;

            //if( confidence < 0.05 ) continue;

            Json::Value result;
            result["track_id"] = track.id;
            result["title"] = track.title;
            result["matches"] = matches;
            result["total"] = file_hash_count;
            result["confidence"] = confidence;

            results.append(result);
        }

        Json::Value json;
        json["results"] = results;

        callback(drogon::HttpResponse::newHttpJsonResponse(json));
    }
}; // TrackCtrl
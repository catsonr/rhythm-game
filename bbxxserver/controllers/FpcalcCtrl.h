#pragma once

#include <filesystem>
#include <cstdio>

#include <drogon/HttpSimpleController.h>
#include <drogon/utils/Utilities.h>

class FpcalcCtrl : public drogon::HttpSimpleController<FpcalcCtrl>
{
public:
    PATH_LIST_BEGIN
        PATH_ADD("/fpcalc", drogon::Post);
    PATH_LIST_END

    std::string execute(const std::string& command)
    {
        std::array<char, 128> buffer;
        std::string result;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if( !pipe ) throw std::runtime_error("[Fpcalc::execute] popen() failed!");
        
        while( fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr ) result += buffer.data();
        
        return result;
    }

    void asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback) override
    {
        drogon::MultiPartParser parser;

        if( parser.parse(req) != 0 )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("[Fpcalc::asyncHandleHttpRequest] failed to parse file upload!\n");
            
            callback(resp);
            return;
        }
        
        std::vector<drogon::HttpFile> files = parser.getFiles();
        if( files.empty() )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("[Fpcalc::asyncHandleHttpRequest] no files uploaded!\n");
            
            callback(resp);
            return;
        }
        if( files.size() > 1 )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("[Fpcalc::asyncHandleHttpRequest] more than one file uploaded!\n");
            
            callback(resp);
            return;
        }
        
        drogon::HttpFile& file = files[0];
        
        std::string uuid = drogon::utils::getUuid();
        std::string temp_file_name = uuid + ".";
        temp_file_name.append(file.getFileExtension());
        
        std::filesystem::path upload_dir = drogon::app().getUploadPath();
        
        std::filesystem::path full_path = upload_dir / temp_file_name;
        
        if( file.saveAs(temp_file_name) != 0 )
        {
            drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k500InternalServerError, drogon::ContentType::CT_TEXT_HTML);
            resp->setBody("[Fpcalc::asyncHandleHttpRequest] failed to save '" + full_path.string() + "' to disk!\n");
            
            callback(resp);
            return;
        }
        
        std::string command = "fpcalc -json \"" + full_path.string() + "\" 2>&1";
        std::string command_output = execute(command);
        
        if( std::filesystem::exists(full_path) ) std::filesystem::remove(full_path);

        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k200OK, drogon::ContentType::CT_APPLICATION_JSON);
        resp->setBody(command_output);
        
        callback(resp);
    }
}; // FpcalcCtrl
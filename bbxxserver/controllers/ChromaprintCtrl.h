#pragma once

#include <drogon/HttpSimpleController.h>

class ChromaprintCtrl : public drogon::HttpSimpleController<ChromaprintCtrl>
{
  public:
    std::string httpfile_to_fingerprint(const drogon::HttpFile& file);

    void asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback) override;
    PATH_LIST_BEGIN
      PATH_ADD("/chromaprint", drogon::Post);
    PATH_LIST_END
};

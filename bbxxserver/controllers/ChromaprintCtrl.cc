#include "ChromaprintCtrl.h"

#include <chromaprint.h>
#include "miniaudio.h"

std::string ChromaprintCtrl::httpfile_to_fingerprint(const drogon::HttpFile& file)
{
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init_default();
    config.format = ma_format_s16;

    ma_result result = ma_decoder_init_memory(
        file.fileData(),
        file.fileLength(),
        &config,
        &decoder
    );
    if( result != MA_SUCCESS )
    {
        printf("[ChromaprintCtrl::httpfile_to_fingerprint] ma_decoder_init_memory() failed!");
        return "n/a :(";
    }
    
    ChromaprintContext* ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST2);
    chromaprint_start(ctx, decoder.outputSampleRate, decoder.outputChannels);
    
    const int CHUNK_FRAMES = 4096;
    std::vector<int16_t> buffer(CHUNK_FRAMES * decoder.outputChannels);
    ma_uint64 frames_read;
    
    while(true)
    {
        result = ma_decoder_read_pcm_frames(&decoder, buffer.data(), CHUNK_FRAMES, &frames_read);
        if(result != MA_SUCCESS) break;
        
        if(frames_read == 0) break;
        
        chromaprint_feed(ctx, buffer.data(), frames_read * decoder.outputChannels);
    }
    
    chromaprint_finish(ctx);

    char* fingerprint = nullptr;
    std::string fingerprint_string = "n/a :(";
    if( chromaprint_get_fingerprint(ctx, &fingerprint) == 1 )
    {
        fingerprint_string = std::string(fingerprint);
        chromaprint_dealloc(fingerprint);
    }
    else
    {
        printf("[ChromaprintCtrl::httpfile_to_fingerprint] chromaprint_get_fingerprint() failed!");
    }
    
    chromaprint_free(ctx);
    ma_decoder_uninit(&decoder);

    return fingerprint_string;
}

void ChromaprintCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback)
{
    drogon::MultiPartParser parser;

    if( parser.parse(req) != 0 )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("[ChromaprintCtrl] failed to parse file upload!\n");
        
        callback(resp);
        return;
    }
    
    std::vector<drogon::HttpFile> files = parser.getFiles();

    if( files.empty() )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("[ChromaprintCtrl] no files uploaded!\n");
        
        callback(resp);
        return;
    }
    
    std::string resp_string;
    for(const drogon::HttpFile& file : files)
    {
        resp_string += file.getFileName() + " is " + std::to_string(file.fileLength()) + " bytes! has fingerprint of:\n\t" + httpfile_to_fingerprint(file) + "\n\n";
    }
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k200OK, drogon::ContentType::CT_TEXT_HTML);
    resp->setBody(resp_string);
    
    callback(resp);
}
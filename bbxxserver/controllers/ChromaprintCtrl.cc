#include "ChromaprintCtrl.h"

#include <chromaprint.h>
#include "miniaudio.h"

static std::vector<uint32_t> decode_fingerprint(const std::string& fingerprint)
{
    uint32_t* raw_ptr = nullptr;
    int raw_size = 0;
    int algorithm = 0;

    int result = chromaprint_decode_fingerprint(
            fingerprint.c_str(),
            fingerprint.size(),
            &raw_ptr,
            &raw_size,
            &algorithm,
            1
    );
    
    if( result != 1 || raw_ptr == nullptr )
    {
        printf("[ChromaprintCtrl::decode_fingerprint] failed to decode fingerprint!");
        return {};
    }
    
    if( algorithm != CHROMAPRINT_ALGORITHM_TEST2 ) printf("[Chromaprint::decode_fingerprint] used algorithm %i ... (expected CHROMAPRINT_ALGORITHM_TEST2=1) ignoring ...", algorithm);
    
    std::vector<uint32_t> raw(raw_ptr, raw_ptr + raw_size);
    chromaprint_dealloc(raw_ptr);
    
    return raw;
}

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
        return {};
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
        resp->setBody("[ChromaprintCtrl::asyncHandleHttpRequest] failed to parse file upload!\n");
        
        callback(resp);
        return;
    }
    
    std::vector<drogon::HttpFile> files = parser.getFiles();

    if( files.empty() )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("[ChromaprintCtrl::asyncHandleHttpRequest] no files uploaded!\n");
        
        callback(resp);
        return;
    }
    if( files.size() > 1 )
    {
        drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::ContentType::CT_TEXT_HTML);
        resp->setBody("[ChromaprintCtrl::asyncHandleHttpRequest] more than one file uploaded!\n");
        
        callback(resp);
        return;
    }
    
    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse(drogon::k200OK, drogon::ContentType::CT_TEXT_HTML);
    resp->setBody( httpfile_to_fingerprint(files[0]) );
    
    callback(resp);
}
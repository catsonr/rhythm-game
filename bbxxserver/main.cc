// taken from drogon examples/helloworld

#include <trantor/utils/Logger.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

#include <drogon/drogon.h>
using namespace drogon;

#include "models/storage.h"

int main()
{
    // `registerHandler()` adds a handler to the desired path. The handler is
    // responsible for generating a HTTP response upon an HTTP request being
    // sent to Drogon
    app().registerHandler(
        "/",
        [](const HttpRequestPtr &request,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            LOG_INFO << "connected:"
                     << (request->connected() ? "true" : "false");
            auto resp = HttpResponse::newHttpResponse();
            resp->setBody("hello from bbxxserver! :)\n");
            callback(resp);
        },
        {Get});

    app()
        .setBeforeListenSockOptCallback([](int fd) {
            LOG_INFO << "setBeforeListenSockOptCallback:" << fd;
#ifdef _WIN32
#elif __linux__
            int enable = 1;
            if (setsockopt(
                    fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable)) ==
                -1)
            {
                LOG_INFO << "setsockopt TCP_FASTOPEN failed";
            }
#else
#endif
        })
        .setAfterAcceptSockOptCallback([](int) {});

    // populate db with some entires
    auto storage = server::init_storage();
    storage.sync_schema();
    if( storage.count<server::Artist>() == 0 )
    {
        // artists
        server::Artist artist;
        artist.name = "Radiohead";
        storage.insert(artist); // artist 1
        artist.name = "Aphex Twin";
        storage.insert(artist); // artist 2
        
        // albums
        server::Album album;
        album.title = "The Bends";
        album.type = server::Album::Type::LP;
        album.ARTIST_ID = 1;
        storage.insert(album); // album 1
        album.title = "Selected Ambient Works 85-92";
        album.type = server::Album::Type::LP;
        album.ARTIST_ID = 2;
        storage.insert(album); // album 2
        
        // tracks
        server::Track track;
        track.title = "Just";
        track.ALBUM_ID = 1;
        storage.insert(track); // track 1
        track.title = "My Iron Lung";
        track.ALBUM_ID = 1;
        storage.insert(track); // track 2
        track.title = "Xtal";
        track.ALBUM_ID = 2;
        storage.insert(track); // track 3
        track.title = "Pulsewidth";
        track.ALBUM_ID = 2;
        storage.insert(track); // track 4
    }

    app().setClientMaxBodySize(20 * 1024*1024); // 20mb

    // Ask Drogon to listen on 127.0.0.1 port 8848. Drogon supports listening
    // on multiple IP addresses by adding multiple listeners. For example, if
    // you want the server also listen on 127.0.0.1 port 5555. Just add another
    // line of addListener("127.0.0.1", 5555)
    LOG_INFO << "server running on 0.0.0.0:3939";
    app().addListener("0.0.0.0", 3939).run();
}
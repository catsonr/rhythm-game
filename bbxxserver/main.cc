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

    // Ask Drogon to listen on 127.0.0.1 port 8848. Drogon supports listening
    // on multiple IP addresses by adding multiple listeners. For example, if
    // you want the server also listen on 127.0.0.1 port 5555. Just add another
    // line of addListener("127.0.0.1", 5555)
    LOG_INFO << "server running on 127.0.0.1:3939";
    app().addListener("127.0.0.1", 3939).run();
}
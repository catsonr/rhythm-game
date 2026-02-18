#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <drogon/WebSocketController.h>

#include "../models/storage.h"

namespace server
{

struct UserLobby
{
private:
    std::shared_mutex shared_mutex;
    std::unordered_map<uint64_t, drogon::WebSocketConnectionPtr> users;

public:
    static UserLobby& get()
    {
        static UserLobby singleton;
        return singleton;
    }

    void add_user(uint64_t USER_ID, const drogon::WebSocketConnectionPtr& wsConnPtr)
    {
        std::unique_lock<std::shared_mutex> lock(shared_mutex);
        users[USER_ID] = wsConnPtr;
        LOG_INFO << "[UserLobby::add_user] user (" + std::to_string(USER_ID) + ") joined lobby! " + std::to_string(users.size()) + " users online";
    }
    
    void remove_user(uint64_t USER_ID, const drogon::WebSocketConnectionPtr& wsConnPtr)
    {
        std::unique_lock<std::shared_mutex> lock(shared_mutex);
        
        auto it = users.find(USER_ID);
        if( it != users.end() && it->second == wsConnPtr )
        {
            users.erase(it);
            LOG_INFO << "[UserLobby::remove_user] user (" + std::to_string(USER_ID) + ") left lobby... " + std::to_string(users.size()) + " users online";
        }
        else LOG_INFO << "[UserLobby::remove_user] ignorning stale connection for user (" + std::to_string(USER_ID) + ")... (assuming connection has already been overwritten)";

    }
    
    void broadcast(const std::string& message)
    {
        std::shared_lock<std::shared_mutex> lock(shared_mutex);
        for( auto const& [id, wsConnPtr] : users )
        {
            if( wsConnPtr->connected() ) wsConnPtr->send(message);
        }
    }
}; // UserLobby

/*
   UserWebSocketCtx is used to cache information about a user who is connected through websocket
   this struct gets attached to the corresponding drogon::WebSocketConnectionPtr (see handleNewConnection())

   UserWebSocketCtx is not to be confused with server::UserSession, which is bbxxserver's list of authenticated
   users (which may or may not be currently online!)
*/
struct UserWebSocketCtx
{
    uint64_t USER_ID;
    std::string username;
}; // UserWebSocketContext

} // server

class UserWebSocketCtrl : public drogon::WebSocketController<UserWebSocketCtrl>
{
public:
    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/ws", "AuthFilter", "ApiFilter");
    WS_PATH_LIST_END

    void handleNewMessage(const drogon::WebSocketConnectionPtr &, std::string &&, const drogon::WebSocketMessageType &) override
    {
    }

    void handleNewConnection(const drogon::HttpRequestPtr &req, const drogon::WebSocketConnectionPtr &wsConnPtr) override
    {
        uint64_t USER_ID = req->attributes()->get<uint64_t>("USER_ID");

        server::Storage storage = server::init_storage();
        std::unique_ptr<server::User> user_ptr = storage.get_pointer<server::User>(USER_ID);
        if( !user_ptr ) // this should never happen thanks to AuthFilter!
        {
            LOG_ERROR << "[UserWebSocketCtrl::handleNewConnection] attempted to handle new web socket connection for user with id (" + std::to_string(USER_ID) + "), but no user exists! closing connection ...";
            wsConnPtr->forceClose();
            return;
        }

        std::shared_ptr<server::UserWebSocketCtx> ctx = std::make_shared<server::UserWebSocketCtx>();
        ctx->USER_ID = USER_ID;
        ctx->username = user_ptr->username;
        wsConnPtr->setContext(ctx);
        
        server::UserLobby& user_lobby = server::UserLobby::get();
        user_lobby.add_user(USER_ID, wsConnPtr);

        user_lobby.broadcast(ctx->username + " joined!");
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &wsConnPtr) override
    {
        std::shared_ptr<server::UserWebSocketCtx> ctx = wsConnPtr->getContext<server::UserWebSocketCtx>();
        
        if( ctx )
        {
            server::UserLobby& user_lobby = server::UserLobby::get();
            user_lobby.get().remove_user(ctx->USER_ID, wsConnPtr);

            user_lobby.get().broadcast(ctx->username + " left...");
        }
        else LOG_ERROR << "[UserWebSocketCtrl::handleConnectionClosed] attempted to handle web socket close, but there was no attached UserWebSocketCtx! ignoring ...";
    }
};

#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>

#include "SceneManager.h"
#include "UserSession.h"

namespace rhythm
{

struct ServerClient : public godot::Control
{
    GDCLASS(ServerClient, Control)

private:
    godot::Ref<rhythm::UserSession> user_session;
    godot::Ref<godot::WebSocketPeer> websocket_peer;
    godot::HTTPRequest* http_request;

public:
    void _ready() override
    {
        http_request = memnew(godot::HTTPRequest);
        http_request->connect("request_completed", callable_mp(this, &ServerClient::_on_request_completed));
        add_child(http_request);
        ping();
        
        websocket_peer.instantiate();
        join_lobby();
    }
    
    void join_lobby()
    {
        if( user_session.is_null() )
        {
            godot::print_error("[ServerClient::join_lobby] cannot join lobby. not user session set!");
            return;
        }
        
        godot::PackedStringArray headers;
        headers.append("Authorization: Bearer " + godot::String(user_session->get_uuid()));
        websocket_peer->set_handshake_headers(headers);
        
        godot::Error e = websocket_peer->connect_to_url("ws://api.beatboxx.org/ws");
        if( e == godot::OK ) godot::print_line("[ServerClient::join_lobby] connecting to lobby ...");
        else godot::print_error("[ServerClient::join_lobby] connect_to_url() failed with an error: " + godot::String::num_int64(e));
    }
    
    void ping()
    {
        if( user_session.is_null() )
        {
            godot::print_error("[ServerClient::ping] cannot ping. no user session set!");
            return;
        }
        
        godot::PackedStringArray headers;
        headers.append("Content-Type: application/json");
        headers.append("Authorization: Bearer " + godot::String(user_session->get_uuid()));
        
        godot::Error e = http_request->request("http://api.beatboxx.org/", headers, godot::HTTPClient::METHOD_GET);
        if( e != godot::OK ) godot::print_error("[ServerClient::ping] request() returned an error: " + godot::String::num_int64(e));
    }
    
    void _on_request_completed(int p_result, int p_response_code, const godot::PackedStringArray& p_headers, const godot::PackedByteArray& p_body)
    {
        if( p_response_code == 530 )
        {
            godot::print_error("[ServerClient::_on_request_completed] bbxxserver responded with status code 530. (is the server running?)");
            return;
        }
        godot::print_line( "[ServerClient::_on_request_completed] http://api.beatboxx.org/ says: " + p_body.get_string_from_utf8() + "\n\twith status code: " + godot::String::num_uint64(p_response_code));
    }
    
    void _process(double delta) override
    {
        websocket_peer->poll();

        godot::WebSocketPeer::State websocket_peer_state = websocket_peer->get_ready_state();
        if( websocket_peer_state == godot::WebSocketPeer::STATE_OPEN )
        {
            while( websocket_peer->get_available_packet_count() > 0 )
            {
                godot::String packet = websocket_peer->get_packet().get_string_from_utf8();
                godot::print_line("[ServerClient::_process] ws://api.beatboxx.org/ says: " + packet);
            }
        }
        //else godot::print_error("[ServerClient::_process] websocket state: " + godot::String::num_uint64(websocket_peer_state));
    }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        
        draw_rect({ 0, 0, w, h }, { 0, 0, 0, 0.5 });
    }
    
    /* GETTERS & SETTERS */

    godot::Ref<rhythm::UserSession> get_user_session() const { return user_session; }
    void set_user_session(const godot::Ref<rhythm::UserSession>& p_user_session) { user_session = p_user_session; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_user_session"), &rhythm::ServerClient::get_user_session);
        godot::ClassDB::bind_method(godot::D_METHOD("set_user_session", "p_user_session"), &rhythm::ServerClient::set_user_session);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "user_session", PROPERTY_HINT_RESOURCE_TYPE, "UserSession"), "set_user_session", "get_user_session");
    }
}; // ServerClient

} // rhythm
#pragma once

#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>

#include "SceneManager.h"
#include "UserSession.h"

namespace rhythm
{

struct BXApi : public godot::Node
{
    GDCLASS(BXApi, Node)

private:
    godot::Ref<rhythm::UserSession> user_session;
    godot::Ref<godot::WebSocketPeer> websocket_peer;

    godot::HTTPRequest* ping_http_request;

public:
    void _ready() override
    {
        // ping
        ping_http_request = memnew(godot::HTTPRequest);
        ping_http_request->connect("request_completed", callable_mp(this, &BXApi::ping_response));
        add_child(ping_http_request);
        
        // websocket
        websocket_peer.instantiate();
    }

    void _process(double delta) override
    {
        websocket_peer->poll();

        godot::WebSocketPeer::State websocket_peer_state = websocket_peer->get_ready_state();
        if( websocket_peer_state == godot::WebSocketPeer::STATE_OPEN )
        {
            if( websocket_peer->get_available_packet_count() > 0 ) godot::print_line("[BXApi::_process] recieved packet(s) from bbxxserver!");

            while( websocket_peer->get_available_packet_count() > 0 )
            {
                godot::String packet = websocket_peer->get_packet().get_string_from_utf8();
                godot::print_line("[BXApi::_process] ws://api.beatboxx.org/ says: " + packet);
            }
        }
        //else godot::print_error("[BXApi::_process] websocket state: " + godot::String::num_uint64(websocket_peer_state));
    }

    void join_lobby()
    {
        if( user_session.is_null() )
        {
            godot::print_error("[BXApi::join_lobby] cannot join lobby. no user session set!");
            return;
        }
        
        godot::PackedStringArray headers;
        headers.append("Authorization: Bearer " + godot::String(user_session->get_uuid()));
        websocket_peer->set_handshake_headers(headers);
        
        godot::Error e = websocket_peer->connect_to_url("ws://api.beatboxx.org/ws");
        if( e == godot::OK ) godot::print_line("[BXApi::join_lobby] connecting to lobby ...");
        else godot::print_error("[BXApi::join_lobby] connect_to_url() failed with an error: " + godot::String::num_int64(e));
    }
    
    void ping()
    {
        godot::Error e = ping_http_request->request("http://beatboxx.org/", {}, godot::HTTPClient::METHOD_GET);
        if( e != godot::OK ) godot::print_error("[BXApi::ping] request() returned an error: " + godot::String::num_int64(e));
    }
    void ping_response(int p_result, int p_response_code, const godot::PackedStringArray& p_headers, const godot::PackedByteArray& p_body)
    {
        if( p_response_code == 530 )
        {
            godot::print_error("[BXApi::ping_response] bbxxserver responded with status code 530. (is the server running?)");
            return;
        }
        godot::print_line( "[BXApi::ping_response] http://beatboxx.org/ says: " + p_body.get_string_from_utf8() + "\n\twith status code: " + godot::String::num_uint64(p_response_code));
    }
    
protected:
    static void _bind_methods() {}
}; // BXApi

} // rhythm
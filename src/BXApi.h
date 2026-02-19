#pragma once

#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/json.hpp>

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
    godot::HTTPRequest* login_http_request;

public:
    void _ready() override
    {
        // ping
        ping_http_request = memnew(godot::HTTPRequest);
        ping_http_request->connect("request_completed", callable_mp(this, &BXApi::ping_response));
        add_child(ping_http_request);

        // login
        login_http_request = memnew(godot::HTTPRequest);
        login_http_request->connect("request_completed", callable_mp(this, &BXApi::login_response));
        add_child(login_http_request);
        
        // websocket
        websocket_peer.instantiate();
        
        user_session.instantiate();
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
        if( e != godot::OK ) godot::print_error("[BXApi::ping] failed to make request: " + godot::String::num_int64(e));
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
    
    void login(godot::String& username, godot::String& password)
    {
        godot::Dictionary data;
        data["username"] = username;
        data["password"] = password;
        godot::String json_string = godot::JSON::stringify(data);
        
        godot::PackedStringArray headers;
        headers.append("Content-Type: application/json");

        godot::Error e = login_http_request->request("http://api.beatboxx.org/login", headers, godot::HTTPClient::METHOD_POST, json_string);
        if( e != godot::OK ) godot::print_error("[BXApi::login] failed to make request: " + godot::String::num_int64(e));
        
        godot::print_line("[BXApi::login] login request sent ...");
    }
    void login_response(int p_result, int p_response_code, const godot::PackedStringArray& p_headers, const godot::PackedByteArray& p_body)
    {
        godot::print_line("[BXApi::login_response] login response recieved!");

        if( p_response_code == 530 )
        {
            godot::print_error("[BXApi::login_response] bbxxserver responded with status code 530. (is the server running?)");
            return;
        }
        
        godot::Variant response_parsed = godot::JSON::parse_string(p_body.get_string_from_utf8());
        if( response_parsed.get_type() != godot::Variant::DICTIONARY )
        {
            godot::print_error("[BXApi::login_response] failed to parse response JSON!");
            return;
        }
        
        godot::Dictionary response = response_parsed;
        if( !response.has("uuid") ) // this should never happen
        {
            godot::print_error("[BXApi::login_response] response did not have field 'uuid'!");
            return;
        }
        
        user_session->set_uuid(response["uuid"]);
        
        godot::print_line("[BXApi::login_response] set user_session to: " + user_session->get_uuid());
    }
    
protected:
    static void _bind_methods() {}
}; // BXApi

} // rhythm
#pragma once

#include <vector>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>

#include "AudioEngine.h"

namespace rhythm
{

struct Constellation : public godot::Resource
{
    GDCLASS(Constellation, Resource)

public:
    godot::TypedArray<Track> tracks;
    std::vector<godot::Ref<godot::Texture2D>> covers;
    std::vector<godot::Vector2i> ids;

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_tracks"), &rhythm::Constellation::get_tracks);
        godot::ClassDB::bind_method(godot::D_METHOD("set_tracks", "p_track"), &rhythm::Constellation::set_tracks);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "tracks", godot::PROPERTY_HINT_TYPE_STRING, godot::String::num(godot::Variant::OBJECT) + "/" + godot::String::num(godot::PROPERTY_HINT_RESOURCE_TYPE) + ":Track"), "set_tracks", "get_tracks");
    }

public:
    bool init()
    {
        covers.clear();
        covers.reserve(tracks.size());

        ids.clear();
        ids.reserve(tracks.size());
        
        for(int i = 0; i < tracks.size(); i++)
        {
            godot::Ref<Track> track = tracks[i];
            
            covers.emplace_back(track->get_album()->get_cover());
            ids.emplace_back(0, i);
        }
        
        return true;
    }
    
    bool is_initialized() const { return !ids.empty() && !covers.empty() && ids.size() == tracks.size() && covers.size() == tracks.size(); }

    godot::TypedArray<Track> get_tracks() const { return tracks; }
    void set_tracks(const godot::TypedArray<Track>& p_tracks) { tracks = p_tracks; }
}; // Constellation

struct Observatory : public godot::Control
{
    GDCLASS(Observatory, Control)

    godot::NodePath audio_engine_path;
    rhythm::AudioEngine* audio_engine { nullptr };
    
    godot::ColorRect* background_shader;
    godot::Ref<godot::ShaderMaterial> background_shader_material;
    
    godot::Vector4 G {
        7, 3,
        5, 1
    }; // vector components are matrix values, row-major
    const float scale_initial { 6 };
    float scale { scale_initial };
    float t { 0 };
    float x_offset { 0 };
    float y_offset { 0 };
    
    godot::Ref<rhythm::Constellation> current_constellation;
    int selected_track_index { 0 };
    godot::RichTextLabel* selected_track_label;

public:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_path"), &rhythm::Observatory::get_audio_engine_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_path", "p_audio_engine_path"), &rhythm::Observatory::set_audio_engine_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_path"), "set_audio_engine_path", "get_audio_engine_path");

        godot::ClassDB::bind_method(godot::D_METHOD("get_background_shader_material"), &rhythm::Observatory::get_background_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_background_shader_material", "p_background_shader_material"), &rhythm::Observatory::set_background_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "background_shader_material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_background_shader_material", "get_background_shader_material");

        godot::ClassDB::bind_method(godot::D_METHOD("get_scale"), &rhythm::Observatory::get_scale);
        godot::ClassDB::bind_method(godot::D_METHOD("set_scale", "p_scale"), &rhythm::Observatory::set_scale);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "scale"), "set_scale", "get_scale");

        godot::ClassDB::bind_method(godot::D_METHOD("get_G"), &rhythm::Observatory::get_G);
        godot::ClassDB::bind_method(godot::D_METHOD("set_G", "p_G"), &rhythm::Observatory::set_G);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR4, "G"), "set_G", "get_G");

        godot::ClassDB::bind_method(godot::D_METHOD("get_current_constellation"), &rhythm::Observatory::get_current_constellation);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_constellation", "p_track"), &rhythm::Observatory::set_current_constellation);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_constellation", godot::PROPERTY_HINT_RESOURCE_TYPE, "Constellation"), "set_current_constellation", "get_current_constellation");
    }

    void _ready() override
    {
        if( audio_engine_path.is_empty() )
        {
            godot::print_line("[Observatory::_ready] a NodePath to AudioEngine has not been set. please set one in the inspector!");

            return;
        }
        
        godot::Node* node = get_node_or_null(audio_engine_path);

        if( node )
            audio_engine = godot::Object::cast_to<rhythm::AudioEngine>(node);
        else
            godot::print_line("[Observatory::_ready] a path to AudioEngine is set but it is invalid");

        background_shader = memnew(godot::ColorRect);
        background_shader->set_name("background_shader");
        background_shader->set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        background_shader->set_draw_behind_parent(true);
        if( background_shader_material.is_valid() ) background_shader->set_material(background_shader_material);
        add_child(background_shader);
        
        selected_track_label = memnew(godot::RichTextLabel);
        selected_track_label->set_name("selected_track_label");
        selected_track_label->set_anchors_and_offsets_preset(godot::Control::PRESET_TOP_WIDE);
        selected_track_label->set_fit_content(true);
        selected_track_label->set_clip_contents(false);
        selected_track_label->set_use_bbcode(true);
        add_child(selected_track_label);
        
        if( current_constellation.is_valid() ) current_constellation->init();
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if(key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo())
        {
            switch(key_event->get_physical_keycode())
            {
                // track selection
                case godot::KEY_UP:
                {
                    selected_track_index = (selected_track_index+1) % current_constellation->get_tracks().size();
                    break;
                }
                case godot::KEY_DOWN:
                {
                    int size = current_constellation->get_tracks().size();
                    selected_track_index = (selected_track_index-1 + size) % size;
                    break;
                }

                // camera movement 
                case godot::KEY_BACKSPACE:
                {
                    x_offset = 0;
                    y_offset = 0;
                    set_scale(scale_initial);
                    break;
                }
                
                default: break;
            }
        }
        else if(key_event.is_valid() && key_event->is_released())
        {
            switch(key_event->get_physical_keycode())
            {
                case godot::KEY_J:
                {
                    break;
                }
                
                default: break;
            }
        }
        
        godot::Ref<godot::InputEventMouseButton> mouse_event = event;
        if(mouse_event.is_valid() && mouse_event->is_pressed())
        {
            float zoom_amount = 1;
            switch(mouse_event->get_button_index())
            {
                    // zoom out when scrolling down
                case godot::MOUSE_BUTTON_WHEEL_DOWN:
                {
                    //godot::print_line("scroll down");
                    set_scale(scale + zoom_amount);
                    break;
                }
                case godot::MOUSE_BUTTON_WHEEL_UP:
                {
                    //godot::print_line("scroll up");
                    float min_zoom_amount = 2; // the minimum value, which is the maximum zoom
                    set_scale( (scale-zoom_amount > min_zoom_amount) ? (scale - zoom_amount) : min_zoom_amount );
                    break;
                }

                default: break;
            }
            
        }
    }
    
    void _process(double delta) override
    {
        if(background_shader_material.is_valid())
        {
            background_shader_material->set_shader_parameter("t", t);
            background_shader_material->set_shader_parameter("x_offset", x_offset);
            background_shader_material->set_shader_parameter("y_offset", y_offset);
            //background_shader->set_size({ 800, 100 }); // does not work
            godot::Vector2 background_shader_size = background_shader->get_size();
            background_shader_material->set_shader_parameter("aspect_ratio", background_shader_size.x / background_shader_size.y);
            background_shader_material->set_shader_parameter("scale", scale);
            background_shader_material->set_shader_parameter("iResolution", get_size());
            background_shader_material->set_shader_parameter("grid_matrix_vector", G);
        }

        godot::Input* input = godot::Input::get_singleton();
        if(input->is_physical_key_pressed(godot::KEY_W)) y_offset -= delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_A)) x_offset -= delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_S)) y_offset += delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_D)) x_offset += delta*scale;
        
        t += delta;

        queue_redraw();
        
    }

    void _draw() override
    {
        // draw half-opaque rectangle around Control border
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        godot::Rect2 background_rect { 0, 0, w, h };
        godot::Color background_color { 0.1, 0.1, 0.1, 0.5 };
        draw_rect(background_rect, background_color, false, 20);

        // nothing to draw if no constellation is loaded
        if(current_constellation.is_null()) return;
        
        // some constants
        float unit = h/scale; // converts from std to Control pixels
        float dropshadow_offset = .1*unit;
        godot::Color dropshadow_color = { 0.2, 0.2, 0.2, 0.8 };

        // draw each track
        godot::TypedArray<Track> tracks = current_constellation->get_tracks();
        for(int i = 0; i < tracks.size(); i++)
        {
            godot::Ref<Track> track = tracks[i];
            
            godot::Vector2i id_G = current_constellation->ids[i]; // integer lattice coordinate, in G basis
            godot::Vector2 id_std { G.x*id_G.x + G.y*id_G.y, G.z*id_G.x + G.w*id_G.y }; // G*id_G -> id_std -- convert from lattice point to std coordinate
            
            // size scale (so that selected track is bigger)
            float size_scale = 1.;
            if( i == selected_track_index ) size_scale = 2.5;

            // position track rect
            // (0,0) is at the center of the screen! (assuming no x or y offset)
            // this must also be true for the shader, of course
            float track_rect_width = size_scale*unit;
            float track_rect_x     = ( id_std.x - x_offset + (scale * (w/h))/2)*unit - track_rect_width/2;
            float track_rect_y     = ( id_std.y - y_offset + (    scale    )/2)*unit - track_rect_width/2;
            godot::Rect2 track_rect { track_rect_x, track_rect_y, track_rect_width, track_rect_width }; // track_rect is a square (which is 'size_scale' units wide) centered on its corresponding lattice point
            // position track dropshadow 
            godot::Rect2 dropshadow_rect = track_rect;
            dropshadow_rect.position.x -= dropshadow_offset;
            dropshadow_rect.position.y += dropshadow_offset;
            
            // position selected track label
            if( i == selected_track_index )
            {
                godot::Vector2 selected_track_label_position = track_rect.position;
                selected_track_label_position.x += size_scale*unit;
                selected_track_label->set_position(selected_track_label_position);
                
                godot::Ref<rhythm::Album> selected_track_album = track->get_album();
                godot::String selected_track_label_text
                {
                    "[wave][rainbow]" + track->get_title() + "[/rainbow][/wave]\n" +
                    selected_track_album->get_artist() + "\n" +
                    selected_track_album->get_title() + " (" + godot::String::num_int64(selected_track_album->get_release_year()) + ")"
                };
                selected_track_label->set_text(selected_track_label_text);
            }
            
            // draw dropshadow
            draw_rect(dropshadow_rect, dropshadow_color);
            // draw cover, if it exists
            if(current_constellation->covers[i].is_valid()) draw_texture_rect(current_constellation->covers[i], track_rect, false);
            // otherwise draw pure red
            else draw_rect(track_rect, { 1, 0, 0, 1 });
        }
    }

    /* GETTERS & SETTERS */

    godot::NodePath get_audio_engine_path() const { return audio_engine_path; }
    void set_audio_engine_path(const godot::NodePath& p_audio_engine_path) {audio_engine_path = p_audio_engine_path; }
    
    godot::Ref<godot::ShaderMaterial> get_background_shader_material() const { return background_shader_material; }
    void set_background_shader_material(const godot::Ref<godot::ShaderMaterial>& p_background_shader_material) { background_shader_material = p_background_shader_material; }
    
    float get_scale() const { return scale; }
    void set_scale(float p_scale) { scale = p_scale; }

    godot::Vector4 get_G() const { return G; }
    void set_G(const godot::Vector4& p_G) { G = p_G; }
    
    godot::Ref<rhythm::Constellation> get_current_constellation() const { return current_constellation; }
    void set_current_constellation(const godot::Ref<rhythm::Constellation>& p_current_constellation) { current_constellation = p_current_constellation; current_constellation->init(); }
}; // Observatory

} // rhythm
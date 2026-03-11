#pragma once

/*
    Observatory is BEATBOXX's idea of a song selector
    currently, it renders a single rhythm::Constellation (which can be though of as a playlist)
    
    Observatory uses BXCTX::G as the basis for its 2D lattice
*/

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/tween.hpp>

#include "nodes/sm/SceneMachine.h"

#include "resources/Constellation.h"

#include "nodes/AudioEngine2.h"

namespace rhythm::sm
{

struct Observatory : public sm::BXScene
{
    GDCLASS(Observatory, sm::BXScene)

    rhythm::AudioEngine2* audio_engine_2 { nullptr };
    
    godot::ColorRect* background_shader;
    godot::Ref<godot::ShaderMaterial> background_shader_material;
    
    godot::ColorRect* adjacency_shader;
    godot::Ref<godot::ShaderMaterial> adjacency_shader_material;
    
    godot::Vector4* G { nullptr };
    const float scale_initial { 6 };
    float scale { scale_initial };
    float t { 0 };
    float x_offset { 0 };
    float y_offset { 0 };

    double trans_t { 0 };
    
    godot::Ref<rhythm::Constellation> current_constellation;
    int selected_track_index { 0 };
    godot::RichTextLabel* selected_track_label;

public:

    void transition_in(const Transition& trans) override
    {
        trans_t = trans.t_normalized();
        //double t_end = trans.t_end;

        // interpolate G
        const godot::Vector4 G_start { 0, 1, 0, -1 };
        const godot::Vector4 G_end   { 3, 3, -3, 3 };
        const godot::Vector4 dG = G_end - G_start;
        
        godot::Vector4 G_interpolated = godot::Tween::interpolate_value(
            G_start,
            dG,
            trans_t,
            1.0,
            godot::Tween::TRANS_CUBIC,
            godot::Tween::EASE_IN_OUT
        );
        
        BXCTX::get().G = G_interpolated;
        
        // interpolate pitch
        audio_engine_2->set_current_track_pitch( 
            godot::Tween::interpolate_value(
                0.5,
                0.5,
                trans_t,
                1.0,
                godot::Tween::TRANS_CUBIC,
                godot::Tween::EASE_IN_OUT
            )
        );
        
        // interpolate scale
        const float scale_initial_inital = 50;
        const float dscale = scale_initial - scale_initial_inital;
        scale = godot::Tween::interpolate_value(
            50,
            dscale,
            trans_t,
            1.0,
            godot::Tween::TRANS_CUBIC,
            godot::Tween::EASE_OUT
        );
    }

    godot::StringName bxname() const override { return "observatory"; }

    void _ready() override
    {
        set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);

        audio_engine_2 = BXCTX::get().audio_engine_2;
        G = &BXCTX::get().G;
        
        if( !background_shader_material.is_valid() ) godot::print_error("[Observatory::_ready] no background shader set! please set one in the inspector");
        background_shader = memnew(godot::ColorRect);
        background_shader->set_name("background_shader");
        background_shader->set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        background_shader->set_draw_behind_parent(true);
        if( background_shader_material.is_valid() ) background_shader->set_material(background_shader_material);
        add_child(background_shader);

        if( !adjacency_shader_material.is_valid() ) godot::print_error("[Observatory::_ready] no adjacency shader set! please set one in the inspector");
        adjacency_shader = memnew(godot::ColorRect);
        adjacency_shader->set_name("adjacency_shader");
        adjacency_shader->set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        adjacency_shader->set_draw_behind_parent(true);
        if( adjacency_shader_material.is_valid() ) adjacency_shader->set_material(adjacency_shader_material);
        add_child(adjacency_shader);
        
        selected_track_label = memnew(godot::RichTextLabel);
        selected_track_label->set_name("selected_track_label");
        selected_track_label->set_anchors_and_offsets_preset(godot::Control::PRESET_TOP_WIDE);
        selected_track_label->set_fit_content(true);
        selected_track_label->set_clip_contents(false);
        selected_track_label->set_use_bbcode(true);
        add_child(selected_track_label);
        
        if( current_constellation.is_valid() )
        {
            current_constellation->cache();

            audio_engine_2->set_current_track(current_constellation->tracks[selected_track_index]);
            audio_engine_2->play_current_track();
        }
        else godot::print_error("[Observatory::current_constellation] no constellation set! ignoring ...");
    }
    
    void _unhandled_input(const godot::Ref<godot::InputEvent>& event) override
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

                    move_to( MULTIPLY_BY_G(G, current_constellation->ids[selected_track_index] ));
                    
                    audio_engine_2->set_current_track(current_constellation->tracks[selected_track_index]);
                    audio_engine_2->play_current_track();

                    break;
                }
                case godot::KEY_DOWN:
                {
                    int size = current_constellation->get_tracks().size();
                    selected_track_index = (selected_track_index-1 + size) % size;

                    move_to( MULTIPLY_BY_G(G, current_constellation->ids[selected_track_index] ));

                    audio_engine_2->set_current_track(current_constellation->tracks[selected_track_index]);
                    audio_engine_2->play_current_track();

                    break;
                }

                // track play/pause
                case godot::KEY_SPACE:
                {
                    if(audio_engine_2->playing_track) audio_engine_2->pause_current_track();
                    else audio_engine_2->play_current_track();
                    
                    break;
                }
                
                case godot::KEY_ENTER:
                {
                    if(key_event->is_shift_pressed()) { SM_TRANSITION(chart_editor) break; }
                    SM_ENTER(diva)
                    
                    break;
                }
                case godot::KEY_ESCAPE:
                {
                    SM_ENTER(title_screen)
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
                    scale += zoom_amount;
                    break;
                }
                case godot::MOUSE_BUTTON_WHEEL_UP:
                {
                    //godot::print_line("scroll up");
                    float min_zoom_amount = 3; // the minimum value, which is the maximum zoom
                    scale = (scale-zoom_amount > min_zoom_amount) ? (scale - zoom_amount) : min_zoom_amount;
                    break;
                }

                default: break;
            }
            
        }
    }
    
    void _process(double delta) override
    {
        godot::Vector2 background_shader_size = background_shader->get_size();
        if(background_shader_material.is_valid())
        {
            background_shader_material->set_shader_parameter("t", t);
            background_shader_material->set_shader_parameter("x_offset", x_offset);
            background_shader_material->set_shader_parameter("y_offset", y_offset);
            background_shader_material->set_shader_parameter("aspect_ratio", background_shader_size.x / background_shader_size.y);
            background_shader_material->set_shader_parameter("scale", scale);
            background_shader_material->set_shader_parameter("iResolution", background_shader_size);
            background_shader_material->set_shader_parameter("grid_matrix_vector", *G);
        }
        if(adjacency_shader_material.is_valid())
        {
            adjacency_shader_material->set_shader_parameter("t", t);
            adjacency_shader_material->set_shader_parameter("x_offset", x_offset);
            adjacency_shader_material->set_shader_parameter("y_offset", y_offset);
            adjacency_shader_material->set_shader_parameter("aspect_ratio", background_shader_size.x / background_shader_size.y);
            adjacency_shader_material->set_shader_parameter("scale", scale);
            adjacency_shader_material->set_shader_parameter("iResolution", background_shader_size);
            adjacency_shader_material->set_shader_parameter("grid_matrix_vector", *G);
            
            adjacency_shader_material->set_shader_parameter("adjacency_texture", current_constellation->observatory_adjacency_shader_texture);
            adjacency_shader_material->set_shader_parameter("adjacency_texture_size", adjacency_shader->get_size());
        }

        godot::Input* input = godot::Input::get_singleton();
        if(input->is_physical_key_pressed(godot::KEY_W)) y_offset -= delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_A)) x_offset -= delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_S)) y_offset += delta*scale;
        if(input->is_physical_key_pressed(godot::KEY_D)) x_offset += delta*scale;
        
        t += delta;

        queue_redraw();
    }
    
    
    /*
        focuses the Observatory to the given point, in std basis
    */
    void move_to(godot::Vector2 p_std) { x_offset = p_std.x; y_offset = p_std.y; }
    
    /* multiplies v by G */
    static godot::Vector2 MULTIPLY_BY_G(godot::Vector4* G, const godot::Vector2& v) { return { G->x*v.x + G->y*v.y, G->z*v.x + G->w*v.y }; }
    
    /*
        maps from the standard basis to the Control basis in pixels with (0, 0) in the center of Control
    */
    static godot::Vector2 std_to_pixel(const godot::Vector2& pos_std, float x_offset, float y_offset, float w, float h, float scale)
    {
        const float unit = h/scale;

        float x_pixel = (pos_std.x - x_offset)*unit + w/2;
        float y_pixel = (pos_std.y - y_offset)*unit + h/2;

        return { x_pixel, y_pixel };
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
            godot::Vector2 id_std = MULTIPLY_BY_G(G, id_G); // G*id_G -> id_std -- convert from lattice point to std coordinate

            // size scale (so that selected track is bigger)
            float size_scale = 1.;
            if( i == selected_track_index ) size_scale = 2.5;

            // track rect width in pixels, according to scale
            float track_rect_width = size_scale*unit;
            // track rect pos in pixels, from lattice coordinate id_std
            godot::Vector2 track_rect_pos = std_to_pixel(id_std, x_offset, y_offset, w, h, scale);
            // track rect pos sin wave animate in based on trans_t
            track_rect_pos.x += (1-trans_t)*w/2;
            track_rect_pos.y += (1-trans_t)*sin(t)*h/2;
            // center track cover on lattice points
            track_rect_pos.x -= track_rect_width / 2;
            track_rect_pos.y -= track_rect_width / 2;
            godot::Rect2 track_rect { track_rect_pos, {track_rect_width, track_rect_width} }; // track_rect is a square (which is 'size_scale' units wide) centered on its corresponding lattice point
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
                    "[outline_color=#222][outline_size=8][wave][rainbow]" + track->get_title() + "[/rainbow][/wave]\n" +
                    selected_track_album->get_artist() + "\n" +
                    selected_track_album->get_title() + " (" + godot::String::num_int64(selected_track_album->get_release_year()) + ")[/outline_size][/outline_color]"
                };
                selected_track_label->set_text(selected_track_label_text);
            }
            
            // position adjacency lines
            // WARN: this only renders a single adjacency!!
            bool has_adjacency = false;
            godot::Vector2 adjacency_start_pos, adjacency_end_pos;
            const std::vector<uint8_t>& adjacencies = current_constellation->adjacencies;
            if( i < tracks.size()-1 && adjacencies[i] != 0 ) // if this track has (a single) adjacency
            {
                has_adjacency = true;
                
                // the change in id, in G basis
                godot::Vector2i d_id_G;

                switch(adjacencies[i])
                {
                    case Constellation::Adjacency::TL:
                    {
                        d_id_G = { -1, -1 };
                        break;
                    }
                    case Constellation::Adjacency::TC:
                    {
                        d_id_G = {  0, -1 };
                        break;
                    }
                    case Constellation::Adjacency::TR:
                    {
                        d_id_G = {  1, -1 };
                        break;
                    }
                    case Constellation::Adjacency::ML:
                    {
                        d_id_G = { -1,  0 };
                        break;
                    }
                    case Constellation::Adjacency::MR:
                    {
                        d_id_G = {  1,  0 };
                        break;
                    }
                    case Constellation::Adjacency::BL:
                    {
                        d_id_G = { -1,  1 };
                        break;
                    }
                    case Constellation::Adjacency::BC:
                    {
                        d_id_G = {  0,  1 };
                        break;
                    }
                    case Constellation::Adjacency::BR:
                    {
                        d_id_G = {  1,  1 };
                        break;
                    }
                }
                
                godot::Vector2 d_id_std = MULTIPLY_BY_G(G, d_id_G); // change in position (in G) -> change in position (in std)!
                
                adjacency_start_pos = track_rect_pos;
                adjacency_start_pos.x += track_rect_width/2;
                adjacency_start_pos.y += track_rect_width/2; // track_rect_pos is shifted off center so that the cover art IS centered. here we just shift it back

                adjacency_end_pos = adjacency_start_pos + d_id_std*unit; // end is start + d_id_pixel
            }
            
            // draw dropshadow
            draw_rect(dropshadow_rect, dropshadow_color);

            // draw adjacency, if it exists
            godot::Color adjacency_color = { sin(t)*sin(t), cos(t)*cos(t), 1, 0.8 };
            if( has_adjacency ) draw_dashed_line(adjacency_start_pos, adjacency_end_pos, adjacency_color, 1, 4 + sin(t/4)*sin(t/4));

            // draw cover, if it exists
            if(current_constellation->covers[i].is_valid()) draw_texture_rect(current_constellation->covers[i], track_rect, false);
            // otherwise draw pure red
            else draw_rect(track_rect, { 1, 0, 0, 1 });
        }
    }

    /* GETTERS & SETTERS */
    
    godot::Ref<godot::ShaderMaterial> get_background_shader_material() const { return background_shader_material; }
    void set_background_shader_material(const godot::Ref<godot::ShaderMaterial>& p_background_shader_material) { background_shader_material = p_background_shader_material; }
    
    godot::Ref<godot::ShaderMaterial> get_adjacency_shader_material() const { return adjacency_shader_material; }
    void set_adjacency_shader_material(const godot::Ref<godot::ShaderMaterial>& p_adjacency_shader_material) { adjacency_shader_material = p_adjacency_shader_material; }
    
    godot::Ref<rhythm::Constellation> get_current_constellation() const { return current_constellation; }
    void set_current_constellation(const godot::Ref<rhythm::Constellation>& p_current_constellation) { current_constellation = p_current_constellation; current_constellation->cache(); }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_background_shader_material"), &rhythm::sm::Observatory::get_background_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_background_shader_material", "p_background_shader_material"), &rhythm::sm::Observatory::set_background_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "background_shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_background_shader_material", "get_background_shader_material");

        godot::ClassDB::bind_method(godot::D_METHOD("get_adjacency_shader_material"), &rhythm::sm::Observatory::get_adjacency_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_adjacency_shader_material", "p_adjacency_shader_material"), &rhythm::sm::Observatory::set_adjacency_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "adjacency_shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_adjacency_shader_material", "get_adjacency_shader_material");

        godot::ClassDB::bind_method(godot::D_METHOD("get_current_constellation"), &rhythm::sm::Observatory::get_current_constellation);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_constellation", "p_track"), &rhythm::sm::Observatory::set_current_constellation);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_constellation", godot::PROPERTY_HINT_RESOURCE_TYPE, "Constellation"), "set_current_constellation", "get_current_constellation");
    }

}; // Observatory

} // rhythm::sm
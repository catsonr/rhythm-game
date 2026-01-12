#pragma once

#include "miniaudio.h"

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/font.hpp>

#include "AudioEngine.h"

namespace rhythm
{

class BeatGraph : public godot::Control
{
    GDCLASS(BeatGraph, Control)

private:
    rhythm::AudioEngine* audio_engine { nullptr };

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine"), &rhythm::BeatGraph::get_audio_engine);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine", "p_audio_engine"), &rhythm::BeatGraph::set_audio_engine);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "audio_engine", PROPERTY_HINT_NODE_TYPE, "AudioEngine"), "set_audio_engine", "get_audio_engine");
    }

public:
    void _ready() override
    {
        if( audio_engine == nullptr ) godot::print_error("[BeatGraph::_ready()] no AudioEngine linked! please set one in the inspector!");
    }
    
    void _process(double delta) override
    {
        queue_redraw();
    }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;

        float line_thickness = 1.5;
        float timeline_radius = 80;

        godot::Rect2 background_rect {0, 0, w, h};
        godot::Color background_color { .2, .2, .2, 0.5 };
        draw_rect(background_rect, background_color, true);
        
        godot::Vector2 axis_start_pos { 0, h/2 };
        godot::Vector2 axis_end_pos { w, h/2 };
        godot::Color axis_color { 0, 0, 0, 0.5 };
        draw_line(axis_start_pos, axis_end_pos, axis_color, line_thickness);
        
        int64_t track_progress_in_frames = audio_engine->get_current_track_progress_in_frames();
        int64_t track_length_in_frames = audio_engine->get_current_track_length_in_frames();
        double track_progress = static_cast<double>(track_progress_in_frames) / static_cast<double>(track_length_in_frames);
        
        float now_line_height = 0.8*h;
        godot::Vector2 now_line_start_pos { w/2, h/2 - now_line_height/2};
        godot::Vector2 now_line_end_pos { w/2, h/2 + now_line_height/2};
        godot::Color now_line_color { 1, 1, 1, 0.5};
        draw_line(now_line_start_pos, now_line_end_pos, now_line_color, line_thickness);
        
        godot::Ref<godot::Font> default_font = godot::ThemeDB::get_singleton()->get_fallback_font();
        draw_string(default_font, {w/2, h - now_line_height}, godot::String::num_int64(track_progress_in_frames), godot::HORIZONTAL_ALIGNMENT_CENTER, 0, 11);
        
        const godot::PackedInt64Array beats = audio_engine->current_track->get_beats();
        
        float beat_line_height = 0.5*h;
        float top_y    = h/2 - beat_line_height/2;
        float bottom_y = h/2 + beat_line_height/2;
        godot::Color beat_line_color { .4, .6, .8, 1 };
        for(int i = 0; i < beats.size(); i++)
        {
            int64_t distance = beats[i] - track_progress_in_frames;
            float distance_screenspace = static_cast<float>(static_cast<double>(distance) / static_cast<double>(timeline_radius));
            
            draw_line({distance_screenspace + w/2, top_y}, {distance_screenspace + w/2, bottom_y}, beat_line_color, line_thickness);
            draw_string(default_font, {distance_screenspace + w/2, bottom_y}, godot::String::num_int64(i), godot::HORIZONTAL_ALIGNMENT_CENTER, 0, 9);
        }
    }

    rhythm::AudioEngine* get_audio_engine() const { return audio_engine; }
    void set_audio_engine(rhythm::AudioEngine* p_audio_engine) { audio_engine = p_audio_engine; }
}; // BeatGraph

} // rhythm
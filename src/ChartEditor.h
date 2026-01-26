#pragma once

#include <godot_cpp/classes/control.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct ChartEditor : public godot::Control
{
    GDCLASS(ChartEditor, godot::Control)

private:
    AudioEngine2* audio_engine_2 { nullptr };
    
    godot::PackedInt64Array current_beats;

protected:
    static void _bind_methods() {}

public:
    void _ready() override
    {
        audio_engine_2 = Scene::conjure_ctx(this)->audio_engine_2;
        
        //audio_engine_2->set_current_track_pitch(1.2);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_BACKSPACE:
                {
                    Scene::conjure_ctx(this)->scene_manager->pop_scene();
                    audio_engine_2->play_current_track();
                    break;
                }
                case godot::KEY_SPACE:
                {
                    if(audio_engine_2->playing_track) audio_engine_2->pause_current_track();
                    else audio_engine_2->play_current_track();
                }
                
                default: break;
            }
        }
    }
    
    void _process(double delta) override
    {
        current_beats = audio_engine_2->current_track->get_beats();

        queue_redraw();
    }
    
    void _draw() override
    {
        // find size of Control
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        
        // background
        godot::Color background_color { .1, .1, .1, 1 };
        draw_rect({ 0, 0, w, h }, background_color);
        
        // find current global and local frames, as well as current pitch 
        const float pitch = audio_engine_2->get_current_track_pitch();
        const rhythm::Conductor& conductor = audio_engine_2->conductor;
        int64_t current_global_frame = ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine);
        int64_t current_local_frame  = (current_global_frame - conductor.GLOBAL_START_FRAME) * pitch;
        if( !audio_engine_2->playing_track ) current_local_frame = conductor.LOCAL_PAUSE_FRAME;
        
        // draw frame-axis
        godot::Color frame_axis_color { 1, 1, 1, 1 };
        draw_line({0, h/2}, {w, h/2}, frame_axis_color, 1);
        
        // draw now line
        double now_line_x = w/2;
        float now_line_height = (h/2)*.9;
        godot::Color now_line_color = frame_axis_color;
        draw_line({(float)now_line_x, h/2 - now_line_height}, {(float)now_line_x, h/2 + now_line_height}, now_line_color, 1);
        
        // draw beats
        godot::Color beat_color { 1, 0.8, 0, 1 };
        const int32_t sample_rate = ma_engine_get_sample_rate(&audio_engine_2->engine);
        const double scale = 200;
        for(int i = 0; i < current_beats.size(); i++)
        {
            int64_t dframes = current_local_frame - current_beats[i];
            double dseconds = static_cast<double>(dframes) / (static_cast<double>(sample_rate) * pitch);
            dseconds *= scale;
            
            double beat_x = now_line_x - dseconds;
            
            float beat_line_height = (h/2)*.2;
            draw_line({(float)beat_x, h/2 - beat_line_height}, {(float)beat_x, h/2 + beat_line_height}, beat_color, 1);
        }
    }
}; // ChartEditor

} // rhythm

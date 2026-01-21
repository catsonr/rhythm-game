#pragma once

#include <vector>
#include <algorithm>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/font.hpp>

#include "SceneManager.h"
#include "Track.h"

namespace rhythm
{

class Taiko : public godot::Control
{
    GDCLASS(Taiko, Control)

private:
    rhythm::AudioEngine* audio_engine { nullptr };
    godot::Color R_color { 1, 0, 1, 1 };
    godot::Color L_color { 0, 1, 1, 1 };
    bool R_pressed, L_pressed; // whether or not L or R is pressed
    bool R_just_pressed, L_just_pressed; // whether or not L or R was just now pressed this frame; will reset on next!
    
    godot::PackedInt64Array beats;
    std::vector<rhythm::Track::Note> notes;
    
    std::vector<int64_t> note_frames;
    int combo { 0 };
    int64_t score;

protected:
    static void _bind_methods() {}

public:
    void _ready() override
    {
        audio_engine = Scene::conjure_ctx(this)->audio_engine;

         beats = audio_engine->current_track->get_beats();
         notes = rhythm::Track::Note::unpack( audio_engine->current_track->get_notes_packed() );
        
        for( const rhythm::Track::Note& note : notes )
        {
            int64_t dt    = beats[note.beat+1] - beats[note.beat];
            int64_t frame = beats[note.beat] + static_cast<int64_t>( static_cast<double>(dt)*note.position );
            
            note_frames.push_back(frame);
        }
        
        godot::print_line("[Taiko::_ready()] loaded frames for ", (int)note_frames.size(), " notes");
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if(key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo())
        {
            switch(key_event->get_physical_keycode())
            {
                case godot::KEY_J:
                {
                    R_pressed = true;
                    R_just_pressed = true;
                    break;
                }
                case godot::KEY_F:
                {
                    L_pressed = true;
                    L_just_pressed = true;
                    break;
                }
                case godot::KEY_SPACE:
                {
                    if(audio_engine->playing_track) audio_engine->pause_current_track();
                    else audio_engine->play_current_track();
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
                    R_pressed = false;
                    break;
                }
                case godot::KEY_F:
                {
                    L_pressed = false;
                    break;
                }
                
                default: break;
            }
        }
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

        float note_width = 100;
        
        // background
        godot::Rect2 background_rect { 0, 0, w, h };
        godot::Color background_color { 0.1, 0.1, 0.1, 1 };
        draw_rect(background_rect, background_color, false, 1);
        
        // R bg highlight
        godot::Rect2 R_rect { w/2, 0, w/2, h };
        godot::Color R_rect_color = R_color;
        R_rect_color.a = 0.2;

        // L bg highlight
        godot::Rect2 L_rect { 0, 0, w/2, h };
        godot::Color L_rect_color = L_color;
        L_rect_color.a = 0.2;
        
        if(R_pressed) draw_rect(R_rect, R_rect_color);
        if(L_pressed) draw_rect(L_rect, L_rect_color);
        
        float judgement_line_y = 0.9*h;
        float judgement_line_width_unpressed = 1;
        float judgement_line_width_pressed = 4;
        draw_line({0,   judgement_line_y}, {w,   judgement_line_y}, {1,1,1,1}, judgement_line_width_unpressed);
        if(L_pressed) draw_line({0,   judgement_line_y}, {w/2, judgement_line_y}, L_color,   judgement_line_width_pressed);
        if(R_pressed) draw_line({w/2, judgement_line_y}, {w,   judgement_line_y}, R_color,   judgement_line_width_pressed);
        
        float scale = 50;
        int64_t current_frame = audio_engine->get_current_track_progress_in_frames();
        for(int i = 0; i < note_frames.size(); i++)
        {
            float note_x;
            godot::Color color;

            if(notes[i].type == rhythm::Track::Note::LEFT)
            {
                note_x = w/2 - note_width;
                color = L_color;
            }
            else
            {
                note_x = w/2;
                color = R_color;
            }
            
            float note_y = judgement_line_y - note_width - (note_frames[i] - current_frame - audio_engine->LATENCY)/scale;
            
            godot::Rect2 rect = { note_x, note_y, note_width, note_width };
            
            draw_rect(rect, color);
        }
        
        // score
        godot::Ref<godot::Font> default_font = godot::ThemeDB::get_singleton()->get_fallback_font();
        godot::String score_string = godot::String::num_int64(score);
        float score_font_size = 10;
        draw_string(default_font, {score_font_size, score_font_size}, score_string, godot::HORIZONTAL_ALIGNMENT_CENTER, 0, score_font_size);
    }
}; // Taiko

} // rhythm
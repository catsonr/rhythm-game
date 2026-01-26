#pragma once

#include <algorithm>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct Taiko2 : public godot::Control
{
    GDCLASS(Taiko2, Control)

private:
    rhythm::AudioEngine2* audio_engine_2 { nullptr };
    godot::Ref<rhythm::Track> track;
    godot::PackedInt64Array beats;
    std::vector<rhythm::Track::Note> notes;
    std::vector<int64_t> note_frames;

    godot::Color R_color { 1, 0, 1, 1 };
    godot::Color L_color { 0, 1, 1, 1 };
    bool R_pressed, L_pressed; // whether or not L or R is pressed
    bool R_just_pressed, L_just_pressed; // whether or not L or R was just now pressed this frame; will reset on next!

    int combo { 0 };
    int next_note_index { 0 };
    uint64_t score { 0 };
    godot::RichTextLabel* score_label;
    
    int64_t current_frame { 0 }; // what local track frame we are on

protected:
    static void _bind_methods() {}

public:
    void _ready() override
    {
        set_clip_contents(true);

        audio_engine_2 = Scene::conjure_ctx(this)->audio_engine_2;
        
        track = audio_engine_2->get_current_track();
        godot::print_line("[Taiko2::_ready] starting w current track '" + track->get_title() + "'!");
        
        beats = audio_engine_2->current_track->get_beats();
        notes = rhythm::Track::Note::unpack( audio_engine_2->current_track->get_notes_packed() );
        
        for( const rhythm::Track::Note& note : notes )
        {
            int64_t dt    = beats[note.beat+1] - beats[note.beat];
            int64_t frame = beats[note.beat] + static_cast<int64_t>( static_cast<double>(dt)*note.position );
            
            note_frames.push_back(frame);
        }
        
        godot::print_line("[Taiko2::_ready] loaded ", (int)note_frames.size(), " notes");
        
        audio_engine_2->set_current_track_pitch(1.0);
        audio_engine_2->set_current_track_progress_in_frames(0);
        audio_engine_2->play_current_track();
        
        // score label
        score_label = memnew(godot::RichTextLabel);
        score_label->set_name("score_label");
        score_label->set_anchors_and_offsets_preset(godot::Control::PRESET_CENTER);
        score_label->set_fit_content(true);
        score_label->set_clip_contents(false);
        add_child(score_label);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_ESCAPE:
                {
                    Scene::conjure_ctx(this)->scene_manager->pop_scene();
                    break;
                }
                case godot::KEY_SPACE:
                {
                    if(audio_engine_2->playing_track) audio_engine_2->pause_current_track();
                    else audio_engine_2->play_current_track();
                    break;
                }
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
                default: break;
            }
        }
        else if( key_event.is_valid() && key_event->is_released() )
        {
            switch( key_event->get_physical_keycode() )
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
        current_frame = ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine) - audio_engine_2->conductor.GLOBAL_START_FRAME;
        if(!audio_engine_2->playing_track) current_frame = audio_engine_2->conductor.LOCAL_PAUSE_FRAME;

        // find the "next note"
        if(next_note_index < notes.size() && current_frame >= note_frames[next_note_index]) next_note_index++;
        
        // handle button press
        if( L_just_pressed || R_just_pressed )
        {
            // choose either the "next note", or the one before it as the "hit note"
            int64_t dt = abs(current_frame - note_frames[next_note_index]);
            int hit_note_index = next_note_index;
            if(next_note_index-1 >= 0)
            {
                int64_t dt_previous_note = abs(current_frame - note_frames[next_note_index-1]);
                
                if(dt_previous_note < dt)
                {
                    hit_note_index--;
                    dt = dt_previous_note;
                }
            }
            
            // the distance (in frames) for a hit
            const int64_t judgement_radius = ma_engine_get_sample_rate(&audio_engine_2->engine) * 0.3;
            // the distance (in frames) for a valid hit
            const int64_t valid_hit_radius = judgement_radius*0.5;

            if(dt <= valid_hit_radius)
            {
                score += dt;
                combo++;

                godot::print_line("valid hit @ note ", hit_note_index);
            }
            else if(dt <= judgement_radius)
            {
                score += dt;
                combo = 0;

                godot::print_line("(invalid) hit @ note ", hit_note_index);
            }
            else
            {
                combo = 0;

                godot::print_line("hit nothing!");
            }
        }
        
        // update score label
        godot::String score_label_text { godot::String::num_int64(combo) + "\n" + godot::String::num_int64(score) };
        score_label->set_text(score_label_text);

        L_just_pressed = false;
        R_just_pressed = false;
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
        draw_rect(background_rect, background_color, true);
        draw_rect(background_rect, background_color, false, 10);
        
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

        // judgement line
        float judgement_line_y = 0.9*h;
        float judgement_line_width_unpressed = 1;
        float judgement_line_width_pressed = 4;
        draw_line({0, judgement_line_y}, {w, judgement_line_y}, {1,1,1,1}, judgement_line_width_unpressed);
        if(L_pressed) draw_line({0,   judgement_line_y}, {w/2, judgement_line_y}, L_color, judgement_line_width_pressed);
        if(R_pressed) draw_line({w/2, judgement_line_y}, {w,   judgement_line_y}, R_color, judgement_line_width_pressed);
        
        float scale = 100;
        
        // draw beats
        for(int i = 0; i < beats.size(); i++)
        {
            double distance = beats[i] - current_frame*audio_engine_2->current_track_pitch;
            float beat_y = judgement_line_y - distance/scale;
            
            float beat_line_thickness = 2;
            godot::Color beat_line_color { 1, 1, 1, 0.2 };
            draw_rect({ 0, beat_y - beat_line_thickness/2, w, beat_line_thickness }, beat_line_color);
        }
        
        // draw notes
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
            
            if(current_frame > note_frames[i]) color *= 0.5; // darken notes that have already occurred
            
            if(i == next_note_index) color = { 1, 1, 1, 1 }; // turn "next note" white
            
            double distance = note_frames[i] - current_frame*audio_engine_2->current_track_pitch;
            float note_y = judgement_line_y - note_width - distance/scale;
            
            godot::Rect2 rect = { note_x, note_y, note_width, note_width };
            draw_rect(rect, color);
        }
    }
}; // Taiko2

} // rhythm
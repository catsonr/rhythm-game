#pragma once

#include <vector>

#include "miniaudio.h"

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/font.hpp>

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

#include "AudioEngine.h"
#include "Track.h"

namespace rhythm
{

class BeatGraph : public godot::Control
{
    GDCLASS(BeatGraph, Control)

private:
    rhythm::AudioEngine* audio_engine { nullptr };

    godot::PackedInt64Array proposed_beats;
    godot::PackedInt64Array current_beats;

    std::vector<rhythm::Track::Note> proposed_notes;
    godot::PackedInt64Array current_notes_packed;

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
        if( audio_engine == nullptr ) godot::print_error("[BeatGraph::_ready] no AudioEngine linked! please set one in the inspector!");
        
        proposed_notes = rhythm::Track::Note::unpack(audio_engine->current_track->get_notes_packed());
        current_beats = audio_engine->current_track->get_beats();
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        /* mouse */

        godot::Ref<godot::InputEventMouseButton> mouse_event = event;
        if(mouse_event.is_valid() && mouse_event->is_pressed())
        {
            const int64_t scroll_speed = 3000;
            float scroll_factor = mouse_event->get_factor();
            int64_t current_progress = audio_engine->get_current_track_progress_in_frames();

            if(mouse_event->get_button_index() == godot::MOUSE_BUTTON_WHEEL_UP)
                audio_engine->set_current_track_progress_in_frames(current_progress + scroll_speed*scroll_factor);
            else if(mouse_event->get_button_index() == godot::MOUSE_BUTTON_WHEEL_DOWN)
                audio_engine->set_current_track_progress_in_frames(current_progress - scroll_speed*scroll_factor);
        }
        
        /* keyboard */
        
        godot::Ref<godot::InputEventKey> key_event = event;
        if(key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo())
        {
            switch(key_event->get_physical_keycode())
            {
                case godot::KEY_SPACE:
                {
                    if(audio_engine->playing_track) audio_engine->pause_current_track();
                    else audio_engine->play_current_track();
                    break;
                }
                case godot::KEY_BACKSPACE:
                {
                    audio_engine->set_current_track_progress_in_frames(0);
                    break;
                }
                case godot::KEY_BRACKETLEFT: // TODO: this doesn't work perfecty .... check audio_engine seeking!
                {
                    if(audio_engine->CURRENT_TRACK_NEXT_BEAT_INDEX - 1 >= 0)
                        audio_engine->set_current_track_progress_in_frames(current_beats[audio_engine->CURRENT_TRACK_NEXT_BEAT_INDEX - 1]);
                    break;
                }
                case godot::KEY_BRACKETRIGHT:
                {
                    int i = audio_engine->CURRENT_TRACK_NEXT_BEAT_INDEX;
                    if(i < current_beats.size())
                    {
                        if(audio_engine->get_current_track_progress_in_frames() == current_beats[i] && i+1 < current_beats.size()) i++;
                        audio_engine->set_current_track_progress_in_frames(current_beats[i]);
                    }
                    break;
                }
                
                // beat inputting
                case godot::KEY_BACKSLASH:
                {
                    proposed_beats.clear();
                    
                    godot::print_line("[BeatGraph::_input] cleared proposed beats");

                    return;
                }
                case godot::KEY_M:
                {
                    int64_t current_track_progress = audio_engine->get_current_track_progress_in_frames();
                    proposed_beats.append(current_track_progress);
                    
                    godot::print_line("[BeatGraph::_input] beat ", proposed_beats.size() - 1, " @ ", current_track_progress, " added to proposition");

                    break;
                }
                case godot::KEY_ENTER:
                {
                    audio_engine->current_track->set_beats(proposed_beats);
                    godot::ResourceSaver::get_singleton()->save(audio_engine->current_track);
                    
                    godot::print_line("[BeatGraph::_input] sent ", proposed_beats.size(), " proposed beats to current track!");
                    
                    break;
                }
                
                // note inputting
                case godot::KEY_ESCAPE:
                {
                    proposed_notes.clear();
                    
                    godot::print_line("[BeatGraph::_input] cleared proposed notes");
                    
                    break;
                }
                case godot::KEY_X:
                {
                    uint32_t beat = audio_engine->CURRENT_TRACK_NEXT_BEAT_INDEX - 1; 
                    int16_t numerator = rhythm::Track::Note::position_to_numerator(0.5);
                    uint16_t type = rhythm::Track::Note::LEFT;
                    
                    proposed_notes.emplace_back(beat, numerator, type);
                    
                    godot::print_line("[BeatGraph::_input] placed an L note @ beat ", beat, ":", numerator);
                    
                    break;
                }
                case godot::KEY_C:
                {
                    uint32_t beat = audio_engine->CURRENT_TRACK_NEXT_BEAT_INDEX - 1; 
                    int16_t numerator = 0;
                    uint16_t type = rhythm::Track::Note::RIGHT;
                    
                    proposed_notes.emplace_back(beat, numerator, type);
                    
                    godot::print_line("[BeatGraph::_input] placed an R note @ beat ", beat, ":", numerator);
                    
                    break;
                }
                case godot::KEY_SHIFT:
                {
                    audio_engine->current_track->set_notes_packed( rhythm::Track::Note::pack(proposed_notes) );
                    godot::ResourceSaver::get_singleton()->save(audio_engine->current_track);
                    
                    godot::print_line("[BeatGraph::_input] sent ", (int)proposed_notes.size(), " proposed notes to current track!");
                    
                    break;
                }
            }
        }
    }
    
    void _process(double delta) override
    {
        current_beats = audio_engine->current_track->get_beats();
        current_notes_packed = audio_engine->current_track->get_notes_packed();
        queue_redraw();
    }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;

        float line_thickness = 1.5;
        float timeline_radius = 80;

        // BACKGROUD
        godot::Rect2 background_rect {0, 0, w, h};
        godot::Color background_color { .2, .2, .2, 0.5 };
        draw_rect(background_rect, background_color, true);
        
        // X-AXIS
        godot::Vector2 axis_start_pos { 0, h/2 };
        godot::Vector2 axis_end_pos { w, h/2 };
        godot::Color axis_color { 1, 1, 1, 0.5 };
        draw_line(axis_start_pos, axis_end_pos, axis_color, line_thickness);
        
        // PROGRESS BAR
        int64_t track_progress_in_frames = audio_engine->get_current_track_progress_in_frames();
        int64_t track_length_in_frames = audio_engine->get_current_track_length_in_frames();
        double track_progress = static_cast<double>(track_progress_in_frames) / static_cast<double>(track_length_in_frames);
        godot::Rect2 progress_rect { 0, static_cast<real_t>(0.9*h), static_cast<real_t>(w*track_progress), static_cast<real_t>(0.1*h) };
        godot::Color progress_color { 1, 1, 1, 0.8 };
        draw_rect(progress_rect, progress_color, true);
        
        // PROGRESS TEXT
        godot::Ref<godot::Font> default_font = godot::ThemeDB::get_singleton()->get_fallback_font();
        godot::String progress_string = godot::String::num_int64(track_progress_in_frames) + " / " + godot::String::num_int64(track_length_in_frames) + " (" + godot::String::num_int64(track_progress*100) + "%)";
        draw_string(default_font, {2, 11}, progress_string, godot::HORIZONTAL_ALIGNMENT_CENTER, 0, 11);

        // NOW LINE (y-intercept)
        float now_line_height = 0.75*h;
        godot::Vector2 now_line_start_pos { w/2, h/2 - now_line_height/2};
        godot::Vector2 now_line_end_pos { w/2, h/2 + now_line_height/2};
        godot::Color now_line_color = axis_color;
        draw_line(now_line_start_pos, now_line_end_pos, now_line_color, line_thickness);
        
        // BEATS
        float beat_line_height = 0.5*h;
        float top_y    = h/2 - beat_line_height/2;
        float bottom_y = h/2 + beat_line_height/2;
        godot::Color beat_line_color { .4, .6, .8, 1 };
        for(int i = 0; i < current_beats.size(); i++)
        {
            int64_t distance = current_beats[i] - track_progress_in_frames;
            float distance_screenspace = static_cast<float>(static_cast<double>(distance) / static_cast<double>(timeline_radius));
            
            draw_line({distance_screenspace + w/2, top_y}, {distance_screenspace + w/2, bottom_y}, beat_line_color, line_thickness);
            draw_string(default_font, {distance_screenspace + w/2, bottom_y}, godot::String::num_int64(i), godot::HORIZONTAL_ALIGNMENT_CENTER, 0, 9);
        }
        
        // NOTES
        std::vector<rhythm::Track::Note> notes = rhythm::Track::Note::unpack( current_notes_packed ); 
        for(const rhythm::Track::Note& note : notes)
        {
            int64_t distance = current_beats[note.beat];
            if(note.beat + 1 < current_beats.size())
                distance += (current_beats[note.beat + 1] - current_beats[note.beat]) * note.position;
            
            distance -= track_progress_in_frames;
            
            float distance_screenspace = static_cast<float>(static_cast<double>(distance) / static_cast<double>(timeline_radius));
            float note_size = 20;
            
            godot::Rect2 note_rect { distance_screenspace - note_size/2 + w/2, h/2 - note_size/2, note_size, note_size };
            godot::Color note_color_R { 1, 0, .5, 1 };
            godot::Color note_color_L { .5, 0, 1, 1 };
            draw_rect(note_rect, note.type == rhythm::Track::Note::RIGHT ? note_color_R : note_color_L);
        }
    }
    
    /* GETTERS & SETTERS */

    rhythm::AudioEngine* get_audio_engine() const { return audio_engine; }
    void set_audio_engine(rhythm::AudioEngine* p_audio_engine) { audio_engine = p_audio_engine; }
}; // BeatGraph

} // rhythm
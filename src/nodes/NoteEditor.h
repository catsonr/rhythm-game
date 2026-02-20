#pragma once

#include <vector>
#include <algorithm>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct NoteEditor : public godot::Control
{
    GDCLASS(NoteEditor, Control)

private:
    AudioEngine2* audio_engine_2 { nullptr };

    float unit { 20 }; // how many pixels tall/wide a single 'unit' is

    godot::PackedInt64Array beats;
    
    std::vector<rhythm::Track::Note> proposed_notes;
    
    godot::Vector2 mouse_pos;

    const int mouse_hover_stave_none { 0 };
    // the stave the mouse is hovered over
    // where 0 is none, -1, -2, -3, -4 are R1, R2, R3, R4, (and 1, 2, 3, 4 for left)
    // choosing this sign convention since +y is DOWN
    int mouse_hover_stave { mouse_hover_stave_none };

    const int mouse_hover_beat_none { -1 };
    // the beat the mouse is hovered over
    int mouse_hover_beat { mouse_hover_beat_none };

    const double mouse_hover_position_none { -1.0 };
    // the position within mouse_hover_beat that the mouse is hovering over
    double mouse_hover_position { mouse_hover_position_none };
    
    double center_x_percent { 0.3 };
    double center_y_percent { 0.5 };
    double zoom { 200 };
    
    godot::Color R_color { 1, 0, 1, 1 };
    godot::Color L_color { 0, 1, 1, 1 };
    
    std::vector<double> positions { 0.0, 0.25, 0.5, 0.75 };
    
public:
    void _ready() override
    {
        audio_engine_2 = Scene::conjure_ctx(this)->audio_engine_2;
        
        if( audio_engine_2->current_track.is_valid() )
        {
            audio_engine_2->decode_current_track();
            beats = audio_engine_2->current_track->get_beats();
            
            proposed_notes = Track::Note::unpack( audio_engine_2->current_track->get_notes_packed() );
        }
    }
    
    void _gui_input(const godot::Ref<godot::InputEvent>& event) override
    {
        // mouse moved 
        godot::Ref<godot::InputEventMouseMotion> event_mouse_motion = event;
        if( event_mouse_motion.is_valid() ) mouse_pos = event_mouse_motion->get_position(); // cache new mouse position
        
        // mouse clicked
        godot::Ref<godot::InputEventMouseButton> event_mouse_button = event;
        if( event_mouse_button.is_valid() && event_mouse_button->is_pressed() )
        {
            switch (event_mouse_button->get_button_index())
            {
                case godot::MOUSE_BUTTON_LEFT:
                {
                    mouse_pos = event_mouse_button->get_position(); // still cache new position
                    handle_click_left();
                    break;
                }
                default: break;
            }
        }
    }

    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_ENTER:
                {
                    audio_engine_2->current_track->set_notes_packed( Track::Note::pack(proposed_notes) );
                    godot::ResourceSaver::get_singleton()->save(audio_engine_2->current_track);
                    
                    godot::print_line("[BeatEditor::_input] sent ", (int)proposed_notes.size(), " proposed notes to current track '", audio_engine_2->get_current_track()->get_title(), "'!");

                    break;
                }

                default: break;
            }
        }
    }
    
    Track::Note::Type stave_index_to_note_type(const int i)
    {
        if(i == mouse_hover_stave_none) { godot::print_error("[NoteEditor::stave_index_to_note_type] requested note type for stave index none! returning R1"); return Track::Note::Type::R1; }

        if(i == -1) return Track::Note::Type::R1;
        if(i == -2) return Track::Note::Type::R2;
        if(i == -3) return Track::Note::Type::R3;
        if(i == -4) return Track::Note::Type::R4;
        if(i ==  1) return Track::Note::Type::L1;
        if(i ==  2) return Track::Note::Type::L2;
        if(i ==  3) return Track::Note::Type::L3;
        if(i ==  4) return Track::Note::Type::L4;
        
        return Track::Note::Type::R1;
    }
    
    void handle_click_left()
    {
        process_mouse_state();
        
        if( mouse_hover_beat == mouse_hover_beat_none || mouse_hover_position == mouse_hover_position_none || mouse_hover_stave == mouse_hover_stave_none) return;
        
        Track::Note mouse_hover_note { static_cast<uint32_t>(mouse_hover_beat), Track::Note::position_to_numerator(mouse_hover_position), stave_index_to_note_type(mouse_hover_stave), 0 };

        if( Track::Note::has_note(proposed_notes, mouse_hover_note) ) proposed_notes = Track::Note::remove_note(proposed_notes, mouse_hover_note);
        else proposed_notes = Track::Note::add_note(proposed_notes, mouse_hover_note);
    }
    
    void process_mouse_state()
    {
        godot::Vector2 size = get_size();
        const double center_y = size.y * center_y_percent;
        const double center_x = size.x * center_x_percent;

        // the mouse's veritcal distance from center
        const double mouse_center_y_dist = mouse_pos.y - center_y;
        // the mouse's horizontal distance from center
        const double mouse_center_x_dist = mouse_pos.x - center_x;

        // calculate which stave the mouse is hovering over
        mouse_hover_stave = static_cast<int>( mouse_center_y_dist/unit );
        if( abs(mouse_hover_stave) > 4 ) mouse_hover_stave = 0;
        
        // calculate which beat the mouse is hovering over
        const int64_t local_current_frame = audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine)); // this is the frame of now_line
        const int64_t local_mouse_hover_dframes = x_to_frame(mouse_center_x_dist);
        const int64_t local_mouse_hover_frame = local_current_frame + local_mouse_hover_dframes; // this then is the frame the mouse is hovered over
        mouse_hover_beat = audio_engine_2->conductor.next_beat_search(local_mouse_hover_frame) - 1;
        
        // calculate which position in mouse_hover_beat the mouse is hovering over
        if( mouse_hover_beat+1 < beats.size() && mouse_hover_beat >= 0 ) // if there is a next beat (and a current one)
        {
            const int64_t local_mouse_hover_beat_to_next_beat_dframes = beats[mouse_hover_beat+1] - beats[mouse_hover_beat];
            const int64_t local_mouse_hover_beat_dist = local_mouse_hover_frame - beats[mouse_hover_beat];
            const double mouse_hover_position_exact = static_cast<double>(local_mouse_hover_beat_dist) / static_cast<double>(local_mouse_hover_beat_to_next_beat_dframes);
            
            // linear search for largest position less than or equal to the exact position
            for( const double position : positions )
            {
                if( mouse_hover_position_exact >= position ) mouse_hover_position = position;
                else break;
            }
        }
        else // not hovered over any beat
        {
            mouse_hover_beat = mouse_hover_beat_none;
            mouse_hover_position = mouse_hover_position_none;
        }
    }

    void _process(double delta) override
    {
        process_mouse_state();
        queue_redraw();
    }
    
    double frame_to_x(const int64_t frame) const { return static_cast<double>(frame) / zoom; }
    int64_t x_to_frame(const double x) const { return static_cast<int64_t>( x*zoom ); }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        float center_y = h * center_y_percent;
        float center_x = w * center_x_percent;
        float lw = 1.1; // line width

        // background
        draw_rect({ 0, 0, size.x, size.y }, { 0.2, 0.2, 0.2, 1 });
        
        // now line
        draw_line({ center_x, 0 }, { center_x, h }, { 1, 1, 1, 1 }, lw);
        
        /*
           notes will be drawn into a piano sheet-music esque staff with R1, R2, R3, R4 in right hand
           and L1, L2, L3, L4 in left hand. each hand has 4 possible positions. those notes will go in
           the spaces created by the staves
           from top of screen to bottom, the "staves" (lines):

            stave_Rtop
            stave_R4
            stave_R3
            stave_R2
            stave_R1

            stave_L1
            stave_L2
            stave_L3
            stave_L4
            stave_Ltop
            
            stave_Rtop and stave_Ltop are just so it looks like a real staff
        */
        float stave_R1   = center_y -   unit;
        float stave_R2   = center_y - 2*unit;
        float stave_R3   = center_y - 3*unit;
        float stave_R4   = center_y - 4*unit;
        float stave_Rtop = center_y - 5*unit;

        float stave_L1   = center_y +   unit;
        float stave_L2   = center_y + 2*unit;
        float stave_L3   = center_y + 3*unit;
        float stave_L4   = center_y + 4*unit;
        float stave_Ltop = center_y + 5*unit;
        
        // mouse hover stave highlight
        if( mouse_hover_stave > 0 ) // stave is left
            draw_rect({ 0, center_y + mouse_hover_stave*unit, w,  unit }, { 1, 1, 1, 0.25 });
        else if( mouse_hover_stave < 0 ) // stave is right
            draw_rect({ 0, center_y + mouse_hover_stave*unit, w, -unit }, { 1, 1, 1, 0.25 });
        
        // staves
        draw_line({ 0, stave_Rtop }, { w, stave_Rtop }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_R4   }, { w, stave_R4   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_R3   }, { w, stave_R3   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_R2   }, { w, stave_R2   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_R1   }, { w, stave_R1   }, { 1, 1, 1, 1 }, lw);

        draw_line({ 0, stave_L1   }, { w, stave_L1   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_L2   }, { w, stave_L2   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_L3   }, { w, stave_L3   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_L4   }, { w, stave_L4   }, { 1, 1, 1, 1 }, lw);
        draw_line({ 0, stave_Ltop }, { w, stave_Ltop }, { 1, 1, 1, 1 }, lw);
        
        // draw beats
        int64_t local_current_frame = audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine));
        const int64_t* beats_ptr = beats.ptr();
        for(int i = 0; i < beats.size(); i++)
        {
            double now_line_x = center_x;
            double beat_dx = frame_to_x(beats_ptr[i] - local_current_frame); // distance from this beat to now line
            float beat_x = static_cast<float>( now_line_x + beat_dx );
            
            godot::Color beats_color { 1, 0.8, 0, 1 };
            draw_line({ beat_x, stave_Rtop }, { beat_x, stave_Ltop }, beats_color, lw);
            
            if( i+1 < beats.size() ) // if there is a beat after this one
            {
                int64_t beat_to_next_beat_dframes = beats_ptr[i+1] - beats_ptr[i];
                double beat_to_next_beat_dx = frame_to_x(beat_to_next_beat_dframes);

                // draw positions
                for(const double position : positions)
                {
                    double position_dx = position*beat_to_next_beat_dx;
                    float position_x = static_cast<float>( beat_x + position_dx );

                    // if this beat and this position is where mouse is hovered
                    if( i == mouse_hover_beat && position == mouse_hover_position && mouse_hover_stave != mouse_hover_stave_none )
                    {
                        float sign = (mouse_hover_stave > 0) ? 1.0 : -1.0;
                        draw_rect({ position_x, mouse_hover_stave*unit + center_y, unit, unit*sign }, { 0, 0, 1, 0.5 });
                    }

                    if( position == 0.0 ) continue;
                    draw_dashed_line({ position_x, stave_Rtop }, { position_x, stave_R1 }, { 1, 1, 1, 1 }, lw);
                    draw_dashed_line({ position_x, stave_L1 }, { position_x, stave_Ltop }, { 1, 1, 1, 1 }, lw);
                }
            }
        }
        
        // draw notes
        for(int i = 0; i < proposed_notes.size(); i++)
        {
            const rhythm::Track::Note& note = proposed_notes[i];
            
            if(note.beat+1 >= beats.size()) continue;

            double now_line_x = center_x;
            double now_line_to_beat_dx = frame_to_x(beats_ptr[note.beat] - local_current_frame);
            double beat_x = now_line_x + now_line_to_beat_dx;
            
            int64_t beat_to_next_beat_dframes = beats_ptr[note.beat+1] - beats_ptr[note.beat];
            double beat_to_next_beat_dx = frame_to_x(beat_to_next_beat_dframes);
            double next_beat_x = beat_x + beat_to_next_beat_dx;
            
            godot::Color note_color;
            double note_y;
            
            switch (note.type)
            {
                // right
                case Track::Note::Type::R1:
                {
                    note_color = R_color;
                    note_y = -2*unit;
                    break;
                }
                case Track::Note::Type::R2:
                {
                    note_color = R_color;
                    note_y = -3*unit;
                    break;
                }
                case Track::Note::Type::R3:
                {
                    note_color = R_color;
                    note_y = -4*unit;
                    break;
                }
                case Track::Note::Type::R4:
                {
                    note_color = R_color;
                    note_y = -5*unit;
                    break;
                }
                // left
                case Track::Note::Type::L1:
                {
                    note_color = L_color;
                    note_y = unit;
                    break;
                }
                case Track::Note::Type::L2:
                {
                    note_color = L_color;
                    note_y = 2*unit;
                    break;
                }
                case Track::Note::Type::L3:
                {
                    note_color = L_color;
                    note_y = 3*unit;
                    break;
                }
                case Track::Note::Type::L4:
                {
                    note_color = L_color;
                    note_y = 4*unit;
                    break;
                }
                default:
                {
                    note_color = { 1, 0, 0, 1 };
                    note_y = center_y;
                    godot::print_error("[NoteEditor::_draw] tried to draw note (index ", i, ") with an invalid Note::Type of ", godot::String::num_int64(note.type));
                    break;
                }
            }
            
            draw_rect({ static_cast<real_t>( beat_x + beat_to_next_beat_dx*note.get_position() ), static_cast<real_t>( center_y + note_y ), unit, unit }, note_color);
        }
        
        // draw beat : position text
        if( mouse_hover_stave != mouse_hover_stave_none )
            draw_string(get_theme_default_font(), mouse_pos, godot::String::num_int64(mouse_hover_beat) + " : " + godot::String::num_real(mouse_hover_position));
    }

protected:
    static void _bind_methods() {}
}; // NoteEditor

} // rhythm

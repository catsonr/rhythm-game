#pragma once

#include <vector>

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/font.hpp>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>

#include <godot_cpp/classes/v_slider.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/check_box.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct BeatEditor : public godot::Control
{
    GDCLASS(BeatEditor, godot::Control)

enum MODE : uint8_t
{
    beats = 0,
    notes = 1,
}; // MODE

private:
    AudioEngine2* audio_engine_2 { nullptr };
    
    // a copy of current_track's beats
    // save to Track with enter key
    godot::PackedInt64Array proposed_beats;

    godot::Color beats_color { 1, 0.8, 0, 1 };
    godot::Color notes_color { 0, 0.8, 1, 1 };
    godot::Color record_color { 1, 0, 0, 1 };

    godot::VSlider* pitch_slider { nullptr };
    godot::HSlider* position_slider { nullptr };
    godot::CheckBox* click_checkbox { nullptr };

    double zoom { 200 };

public:
    void _ready() override
    {
        audio_engine_2 = Scene::conjure_ctx(this)->audio_engine_2;
        
        if( audio_engine_2->current_track.is_valid() )
        {
            audio_engine_2->decode_current_track(); // load entire track into memory so that scrubbing as no delay (otherwise conductor would fall out of time!)

            proposed_beats = audio_engine_2->current_track->get_beats();
        }
        else godot::print_error("[BeatEditor::_ready] current track is not valid!");
        
        pitch_slider = memnew(godot::VSlider);
        pitch_slider->set_min(0.1);
        pitch_slider->set_max(3.0);
        pitch_slider->set_step(0.1);
        pitch_slider->set_value(audio_engine_2->get_current_track_pitch());
        pitch_slider->set_position({10, 10});
        pitch_slider->set_size({10, 100});
        pitch_slider->set_tooltip_text("changes the speed of the track");
        pitch_slider->set_focus_mode(Control::FOCUS_NONE);
        pitch_slider->connect("value_changed", godot::Callable(this, "on_pitch_slider_changed"));
        add_child(pitch_slider);
        
        position_slider = memnew(godot::HSlider);
        position_slider->set_min(0);
        position_slider->set_max(audio_engine_2->get_current_track_length_in_frames());
        position_slider->set_step(1.0);
        position_slider->set_anchors_and_offsets_preset(Control::PRESET_CENTER_BOTTOM);
        position_slider->set_size({400, 10});
        position_slider->set_position({-200, -40});
        position_slider->set_tooltip_text("scrubs through the track");
        position_slider->set_focus_mode(Control::FOCUS_NONE);
        position_slider->connect("value_changed", godot::Callable(this, "on_position_slider_changed"));
        add_child(position_slider);
        
        click_checkbox = memnew(godot::CheckBox);
        click_checkbox->set_text("click");
        click_checkbox->set_anchors_and_offsets_preset(Control::PRESET_TOP_LEFT);
        click_checkbox->set_size({20, 20});
        click_checkbox->set_tooltip_text("whether or not to play a click sound when a beat happens");
        click_checkbox->set_focus_mode(Control::FOCUS_NONE);
        click_checkbox->set_pressed(audio_engine_2->play_click);
        click_checkbox->connect("toggled", godot::Callable(this, "on_click_checkbox_changed"));
        add_child(click_checkbox);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventMouseButton> mouse_event = event;
        if( mouse_event.is_valid() && mouse_event->is_pressed() )
        {
            const int64_t scroll_speed_in_frames { 5000 };
            const double zoom_speed { 20 };;
            const double zoom_min { 10 };
            const double zoom_max { 2000 };
            switch(mouse_event->get_button_index())
            {
                case godot::MOUSE_BUTTON_WHEEL_LEFT: // (all?) OSes treat shift+mouse wheel up/down as mouse wheel left/right
                case godot::MOUSE_BUTTON_WHEEL_DOWN:
                {
                    if( mouse_event->is_shift_pressed() )
                    {
                        zoom += zoom_speed;
                        if( zoom > zoom_max ) zoom = zoom_max;
                    }
                    else
                    {
                        int64_t local_current_frame = audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine));
                        audio_engine_2->set_current_track_progress_in_frames(local_current_frame + scroll_speed_in_frames);
                    }

                    break;
                }
                case godot::MOUSE_BUTTON_WHEEL_RIGHT:
                case godot::MOUSE_BUTTON_WHEEL_UP:
                {
                    if( mouse_event->is_shift_pressed() )
                    {
                        
                        zoom -= zoom_speed;
                        if( zoom < zoom_min ) zoom = zoom_min;
                    }
                    else
                    {
                        int64_t local_current_frame = audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine));
                        audio_engine_2->set_current_track_progress_in_frames(local_current_frame - scroll_speed_in_frames);
                    }

                    break;
                }
                
                default: break;
            }
        }

        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            const int64_t nudge_speed_in_frames { 500 };

            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_ESCAPE:
                {
                    audio_engine_2->conductor.set_beats(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine), audio_engine_2->current_track->get_beats());
                    Scene::conjure_ctx(this)->scene_manager->pop_scene();
                    break;
                }
                case godot::KEY_SPACE:
                {
                    if(audio_engine_2->playing_track) audio_engine_2->pause_current_track();
                    else audio_engine_2->play_current_track();
                    
                    break;
                }
                case godot::KEY_ENTER:
                {
                    audio_engine_2->current_track->set_beats(proposed_beats);
                    godot::ResourceSaver::get_singleton()->save(audio_engine_2->current_track);
                    
                    godot::print_line("[BeatEditor::_input] sent ", (int)proposed_beats.size(), " proposed beats to current track '", audio_engine_2->get_current_track()->get_title(), "'!");

                    break;
                }
                case godot::KEY_X:
                {
                    if(audio_engine_2->conductor.next_beat_index >= 1)
                        proposed_beats = Track::delete_beat_at_index(proposed_beats, audio_engine_2->conductor.next_beat_index-1);
                    
                    audio_engine_2->conductor.set_beats(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine), proposed_beats);
                    break;
                }
                case godot::KEY_M:
                {
                    int64_t local_current_frame = audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine));
                    proposed_beats = Track::insert_beat_at_frame(proposed_beats, local_current_frame);

                    audio_engine_2->conductor.set_beats(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine), proposed_beats);
                    
                    break;
                }
                case godot::KEY_COMMA:
                {
                    proposed_beats = Track::nudge_beat_at_index(proposed_beats, audio_engine_2->conductor.next_beat_index-1, -nudge_speed_in_frames);
                    audio_engine_2->conductor.set_beats(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine), proposed_beats);

                    break;
                }
                case godot::KEY_PERIOD:
                {
                    proposed_beats = Track::nudge_beat_at_index(proposed_beats, audio_engine_2->conductor.next_beat_index-1, nudge_speed_in_frames);
                    audio_engine_2->conductor.set_beats(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine), proposed_beats);

                    break;
                }
                
                default: break;
            }
        }
    }
    
    void _process(double delta) override
    {
        position_slider->set_value_no_signal(audio_engine_2->conductor.get_local_current_frame(ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine)));
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
        int64_t global_current_frame = ma_engine_get_time_in_pcm_frames(&audio_engine_2->engine);
        int64_t local_current_frame  = conductor.get_local_current_frame(global_current_frame);
        
        // draw frame-axis (x-axis)
        godot::Color frame_axis_color { 1, 1, 1, 1 };
        draw_line({0, h/2}, {w, h/2}, frame_axis_color, 1.5);
        
        // draw now line
        double now_line_x = w/2;
        float now_line_height = (h/2)*.9;
        godot::Color now_line_color = frame_axis_color;
        draw_line({(float)now_line_x, h/2 - now_line_height}, {(float)now_line_x, h/2 + now_line_height}, now_line_color, 1.5);
        
        // draw beats
        const int32_t sample_rate = ma_engine_get_sample_rate(&audio_engine_2->engine);
        for(int i = 0; i < proposed_beats.size(); i++)
        {
            int64_t dframes = local_current_frame - proposed_beats[i];
            double dseconds = static_cast<double>(dframes) / (static_cast<double>(sample_rate) * pitch);
            double beat_x = now_line_x - dseconds*zoom;
            
            float beat_line_height = (h/2)*.2;
            const float current_beat_height_increase = h*0.1;
            
            // this beat is the current one! (and also not the last one)
            if( conductor.next_beat_index-1 == i && conductor.next_beat_index < proposed_beats.size() )
            {
                godot::Color beats_highlight_color = beats_color;
                beats_highlight_color.a = 0.5;
                
                int64_t next_dframes = proposed_beats[i+1] - proposed_beats[i];
                double next_dseconds = static_cast<double>(next_dframes) / (static_cast<double>(sample_rate) * pitch);
                double next_beat_x = beat_x + next_dseconds*zoom;
                
                godot::Vector2 next_beat_rect_pos { static_cast<real_t>(beat_x), h/2 - beat_line_height };
                godot::Vector2 next_beat_rect_size { static_cast<real_t>(next_beat_x-beat_x), beat_line_height*2 };
                
                draw_rect({ next_beat_rect_pos.x, next_beat_rect_pos.y, next_beat_rect_size.x, next_beat_rect_size.y }, beats_highlight_color);

                draw_line({(float)beat_x, h/2 - beat_line_height - current_beat_height_increase}, {(float)beat_x, h/2 + beat_line_height + current_beat_height_increase}, beats_color, 2.5);
                draw_string(get_theme_default_font(), { (float)beat_x, h/2 + beat_line_height + current_beat_height_increase }, godot::String::num_int64(i), HORIZONTAL_ALIGNMENT_CENTER);
            }
            else draw_line({(float)beat_x, h/2 - beat_line_height }, {(float)beat_x, h/2 + beat_line_height }, beats_color, 1.5);

        }
    }

    void on_pitch_slider_changed(double p_value)
    {
        audio_engine_2->set_current_track_pitch(p_value);
    }

    void on_position_slider_changed(double p_value)
    {
        audio_engine_2->set_current_track_progress_in_frames(static_cast<int64_t>(p_value));
    }

    void on_click_checkbox_changed(bool p_toggle_mode)
    {
        audio_engine_2->play_click = p_toggle_mode;
    }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("on_pitch_slider_changed", "value"), &rhythm::BeatEditor::on_pitch_slider_changed);
        godot::ClassDB::bind_method(godot::D_METHOD("on_position_slider_changed", "value"), &rhythm::BeatEditor::on_position_slider_changed);
        godot::ClassDB::bind_method(godot::D_METHOD("on_click_checkbox_changed", "toggled_on"), &rhythm::BeatEditor::on_click_checkbox_changed);
    }

}; // BeatEditor

} // rhythm

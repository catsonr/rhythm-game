#pragma once

/*
    TODO: of course, should be renamed--at some point--to AudioEngine
*/

#include <list>
#include <algorithm>
#include <map>

#include "miniaudio.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "Track.h"
#include "Conductor.h"

namespace rhythm
{

struct AudioEngine2 : public godot::Node
{
    GDCLASS(AudioEngine2, Node)

private:
    public: ma_engine engine; private:
    std::list<ma_sound> sounds;
    
    float volume { 1.0 };
    
    public: godot::Ref<rhythm::Track> current_track; private:
    public: bool playing_track { false }; private:
    public: float current_track_pitch { 1.0 }; private:

    public: Conductor conductor; private:
    godot::Ref<rhythm::Audio> click;
    public: bool play_click { true }; private:
    int next_click_index { 0 };
    int64_t click_latency { 2000 };
    // used to keep track of where Conductor was when playing a Track, so that it can be switched back to that position
    // key is the AudioEngine_sounds_index (see Audio.h), and value is the last frame in local time (see Conductor.h)
    std::map<int, int64_t> conductor_positions;

protected:
    static void _bind_methods()
    {
        // volume
        godot::ClassDB::bind_method(godot::D_METHOD("get_volume"), &rhythm::AudioEngine2::get_volume);
        godot::ClassDB::bind_method(godot::D_METHOD("set_volume", "p_track_pitch"), &rhythm::AudioEngine2::set_volume);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "volume"), "set_volume", "get_volume");

        // current track
        /*
        godot::ClassDB::bind_method(godot::D_METHOD("get_current_track"), &rhythm::AudioEngine2::get_current_track);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_track", "p_track"), &rhythm::AudioEngine2::set_current_track);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_track", godot::PROPERTY_HINT_RESOURCE_TYPE, "Track"), "set_current_track", "get_current_track");
        */

        // current track pitch
        godot::ClassDB::bind_method(godot::D_METHOD("get_current_track_pitch"), &rhythm::AudioEngine2::get_current_track_pitch);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_track_pitch", "p_track_pitch"), &rhythm::AudioEngine2::set_current_track_pitch);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "current_track_pitch"), "set_current_track_pitch", "get_current_track_pitch");

        // click
        godot::ClassDB::bind_method(godot::D_METHOD("get_click"), &rhythm::AudioEngine2::get_click);
        godot::ClassDB::bind_method(godot::D_METHOD("set_click", "p_track"), &rhythm::AudioEngine2::set_click);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "click", godot::PROPERTY_HINT_RESOURCE_TYPE, "Audio"), "set_click", "get_click");

        // click_latency
        godot::ClassDB::bind_method(godot::D_METHOD("get_click_latency"), &rhythm::AudioEngine2::get_click_latency);
        godot::ClassDB::bind_method(godot::D_METHOD("set_click_latency", "p_click_latency"), &rhythm::AudioEngine2::set_click_latency);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "click_latency"), "set_click_latency", "get_click_latency");
    }

public:
    /* GODOT OVERRIDES */
    
    void _exit_tree() override
    {
        pause_current_track();

        for(ma_sound& sound : sounds) ma_sound_uninit(&sound);
        sounds.clear();

        ma_engine_uninit(&engine);
    }

    void _ready() override
    {
        // miniaudio
        if( ma_engine_init(NULL, &engine) != MA_SUCCESS )
        {
            godot::print_error("[AudioEngine2::_ready] failed to initialize miniaudio engine!");
            return;
        }
        
        ma_engine_set_volume(&engine, volume);
        
        // click 
        if(click.is_valid()) load_audio(click);
        else godot::print_line("[AudioEngine2::_ready] tried to load click Audio but one was not set. please set one in the inspector!");
        
        // current track
        if(current_track.is_valid())
        {
            set_current_track(current_track);
            set_current_track_pitch(current_track_pitch);
            
            godot::print_line("[AudioEngine2::_ready] initialized with track '", current_track->get_title(), "' @ pitch = ", current_track_pitch);
        }

        godot::print_line("[AudioEngine2::_ready] miniaudio initialized : )");
    }
    
    void _process(double delta) override
    {
        if(!current_track.is_valid() || !playing_track || !current_track->loaded) return;
        if(ma_sound_at_end(current_track->sound))
        {
            godot::print_line("[AudioEngine2::_process] song ended!");

            pause_current_track();
            conductor_positions.erase(current_track->AudioEngine2_sounds_index);
            set_current_track_progress_in_frames(0);

            return;
        }
        
        /* track is playing */

        conductor.process(ma_engine_get_time_in_pcm_frames(&engine));
        
        // play click sound based of conductor (only audible if play_click is true)
        const Conductor& c = conductor;
        const int next_beat_index = c.next_beat_index;
        const godot::PackedInt64Array& beats = c.beats;
        const int64_t global_current_frame = ma_engine_get_time_in_pcm_frames(&engine);
        const int64_t local_current_frame = c.get_local_current_frame(global_current_frame);
        
        if( next_click_index < beats.size() )
        {
            const int64_t global_next_beat_frame = c.global_start_frame + static_cast<int64_t>( beats[next_click_index] / c.pitch );

            if( global_current_frame >= global_next_beat_frame - click_latency )
            {
                if( play_click )
                {
                    ma_sound_seek_to_pcm_frame(click->sound, 0);
                    ma_sound_set_start_time_in_pcm_frames(click->sound, global_next_beat_frame - click_latency);
                    ma_sound_start(click->sound);
                }

                next_click_index++;
            }
        }
    }
    
    /* PUBLIC METHODS */
    
    bool load_sound(const godot::StringName& path)
    {
        sounds.emplace_back();
        ma_sound& sound = sounds.back();
        
        godot::String abs_path = godot::ProjectSettings::get_singleton()->globalize_path(path);
        godot::CharString abs_path_charstring = abs_path.utf8();

        const ma_uint32 LOAD_FLAGS =
            MA_SOUND_FLAG_NO_SPATIALIZATION |
            MA_SOUND_FLAG_DECODE
            //MA_SOUND_FLAG_ASYNC
        ;

        if( ma_sound_init_from_file(&engine, abs_path_charstring.get_data(), LOAD_FLAGS, NULL, NULL, &sound) != MA_SUCCESS )
        {
            godot::print_error("[AudioEngine2::load_sound] unable to load ", abs_path, "!");
            return false;
        }
        
        //godot::print_line("[AudioEngine2::load_sound] ", abs_path, " loaded as track ", (int)(sounds.size()-1), "!");
        
        return true;
    }
    
    bool load_audio(const godot::Ref<rhythm::Audio>& audio)
    {
        if( audio->loaded )
        {
            //godot::print_line("[AudioEngine2::load_audio] ", audio->get_file_path(), " is already loaded! ignoring ...");
            return true;
        }
        if( !load_sound(audio->get_file_path()) )
        {
            godot::print_error("[AudioEngine2::load_audio] failed to load audio '", audio->get_file_path(), "'!");
            return false;
        }
        
        audio->sound = &sounds.back();
        audio->AudioEngine2_sounds_index = sounds.size()-1;
        audio->loaded = true;

        return true;
    }
    
    void play_current_track()
    {
        if(current_track.is_valid() && current_track->loaded && !playing_track)
        {
            ma_sound_start(current_track->sound);
            playing_track = true;
            
            conductor.play(ma_engine_get_time_in_pcm_frames(&engine));
        } else godot::print_line("[AudioEngine2::play_current_track] nothing to do ...");
    }
    
    void pause_current_track()
    {
        if(current_track.is_valid() && current_track->loaded && playing_track)
        {
            ma_sound_stop(current_track->sound);
            playing_track = false;

            conductor.pause(ma_engine_get_time_in_pcm_frames(&engine));
        } else godot::print_line("[AudioEngine2::pause_current_track] nothing to do ...");
    }

    /* GETTERS & SETTERS */
    
    // volume
    float get_volume() const { return volume; }
    void set_volume(const float p_volume) { volume = p_volume; if(is_node_ready()) ma_engine_set_volume(&engine, volume); }

    // current track
    godot::Ref<rhythm::Track> get_current_track() const { return current_track; }
    void set_current_track(const godot::Ref<rhythm::Track>& p_current_track)
    {
        if(current_track.is_valid() && current_track->loaded)
        {
            pause_current_track();
            // save last position
            conductor_positions[current_track->AudioEngine2_sounds_index] = conductor.pause_frame();
        }

        const godot::Ref<rhythm::Track>& previous_track = current_track;
        current_track = p_current_track;
        if(is_node_ready())
        {
            load_audio(current_track);
            
            int64_t global_current_frame = ma_engine_get_time_in_pcm_frames(&engine);
            
            auto it = conductor_positions.find(current_track->AudioEngine2_sounds_index);
            int64_t global_resume_frame = (it != conductor_positions.end()) ? it->second : 0;

            conductor.seek(global_current_frame, global_resume_frame);
            conductor.set_beats(global_current_frame, current_track->get_beats());
            
            set_current_track_pitch(current_track_pitch);
            
            next_click_index = conductor.next_beat_index;
        }
    }
    
    // current_track_pitch
    float get_current_track_pitch() const { return current_track_pitch; }
    void set_current_track_pitch(const float p_current_track_pitch)
    {
        current_track_pitch = p_current_track_pitch;

        if(is_node_ready() && current_track.is_valid())
        {
            ma_sound_set_pitch(current_track->sound, current_track_pitch);
            conductor.set_pitch(ma_engine_get_time_in_pcm_frames(&engine), p_current_track_pitch);
        }
    }

    // click
    godot::Ref<rhythm::Track> get_click() const { return click; }
    void set_click(const godot::Ref<rhythm::Audio>& p_click) { click = p_click; if(is_node_ready()) load_audio(click); }
    
    // click_latency
    uint64_t get_click_latency() const { return click_latency; }
    void set_click_latency(const uint64_t p_click_latency) { click_latency = p_click_latency; }

    int64_t get_current_track_length_in_frames() const
    {
        ma_uint64 ma_track_length_in_frames;
        ma_sound_get_length_in_pcm_frames(current_track->sound, &ma_track_length_in_frames);
        
        return static_cast<int64_t>(ma_track_length_in_frames);
    }
    
    /*
        WARN: this name is a bit of a misnomer; it returns the frame that miniaudio is reading, not
        the frame we are on. for that, use Conductor::get_local_current_frame()
    */
    private:
    int64_t get_current_track_progress_in_frames() const
    {
        if(!current_track.is_valid() || !current_track->loaded) return 0;
        
        return (int64_t)ma_sound_get_time_in_pcm_frames(current_track->sound);
    }
    public:
    
    void set_current_track_progress_in_frames(int64_t frame)
    {
        if(!current_track.is_valid() || !current_track->loaded) return;

        frame = (frame > get_current_track_length_in_frames()) ? get_current_track_length_in_frames() : frame;
        
        ma_sound_seek_to_pcm_frame(current_track->sound, (ma_uint64)frame);
        conductor.seek(ma_engine_get_time_in_pcm_frames(&engine), frame); // again, this is a place where we are going off of the miniaudio read head for seeking .... (though it seems to work fine ?)
        
        next_click_index = conductor.next_beat_index;
    }
    
    double get_current_track_progress() const { return static_cast<double>(get_current_track_progress_in_frames()) / static_cast<double>(get_current_track_length_in_frames()); }

}; // AudioEngine2

} // rhythm
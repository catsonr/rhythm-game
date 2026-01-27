#pragma once

#include <list>
#include <algorithm>

#include "miniaudio.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "Track.h"

namespace rhythm
{

/* CONDUCTOR */

struct Conductor
{
    /* STATE */

    static constexpr int64_t initial { -1 };
    int64_t global_start_frame { initial };
    int64_t local_pause_frame  { initial };
    double pitch { 1.0 };
    
    godot::PackedInt64Array beats;
    int next_beat_index { 0 };
    
    /* LEMMAS (known values that do not modify state) */

    bool is_playing() const { return global_start_frame != initial; }
    bool is_paused()  const { return local_pause_frame  != initial; }
    int64_t pause_frame() const { return (local_pause_frame != initial) ? local_pause_frame : 0; }
    int64_t get_local_current_frame(const int64_t global_current_frame) const
    {
        // if track is playing, the local current frame is elapsed frames times pitch
        if( is_playing() ) return static_cast<int64_t>( (global_current_frame - global_start_frame)*pitch );
        // otherwise we're paused, where we already know the frame we paused on (or we're at initial, which is frame 0)
        return pause_frame();
    }
    int next_beat_search(int64_t local_frame) const
    {
        const int64_t* start = beats.ptr();
        const int64_t* end   = start + beats.size();

        const int64_t* it = std::upper_bound(start, end, local_frame);
        return static_cast<int>( it-start );
    }
    
    /* OPERATIONS (actions that change state) */
    
    void set_beats(const int64_t global_current_frame, const godot::PackedInt64Array& p_beats)
    {
        beats = p_beats;
        next_beat_index = next_beat_search( get_local_current_frame(global_current_frame) );
    }
    
    void process(const int64_t global_current_frame)
    {
        if( beats.is_empty() || next_beat_index >= beats.size() || is_paused() ) return;
        
        int64_t local_current_time = get_local_current_frame(global_current_frame);
        while( next_beat_index < beats.size() && local_current_time >= beats[next_beat_index] )
        {
            godot::print_line("[Conductor::process] beat ", next_beat_index, "!");

            next_beat_index++;
        }
    }
    
    void play(const int64_t global_current_frame)
    {
        if( is_playing() ) return;

        global_start_frame = global_current_frame - static_cast<int64_t>( pause_frame()/pitch );
        // playing always sets local pause frame back to initial
        local_pause_frame = initial;
    }
    
    void pause(const int64_t global_current_frame)
    {
        if( is_paused() ) return;

        local_pause_frame = get_local_current_frame(global_current_frame);
        // pausing always sets global start frame back to initial
        global_start_frame = initial;
    }
    
    void seek(const int64_t global_current_frame, const int64_t to_local_frame)
    {
        if( is_paused() ) local_pause_frame = to_local_frame;
        else global_start_frame = global_current_frame - static_cast<int64_t>( to_local_frame/pitch );
        
        next_beat_index = next_beat_search(to_local_frame);
    }
    
    void set_pitch(const int64_t global_current_frame, const double p_pitch)
    {
        if( pitch == p_pitch ) return;

        if( is_playing() ) global_start_frame = global_current_frame - static_cast<int64_t>( get_local_current_frame(global_current_frame)/p_pitch );
        
        pitch = p_pitch;
    }
}; // Conductor

/* AUIOENGINE */

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

protected:
    static void _bind_methods()
    {
        // volume
        godot::ClassDB::bind_method(godot::D_METHOD("get_volume"), &rhythm::AudioEngine2::get_volume);
        godot::ClassDB::bind_method(godot::D_METHOD("set_volume", "p_track_pitch"), &rhythm::AudioEngine2::set_volume);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "volume"), "set_volume", "get_volume");

        // current track
        godot::ClassDB::bind_method(godot::D_METHOD("get_current_track"), &rhythm::AudioEngine2::get_current_track);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_track", "p_track"), &rhythm::AudioEngine2::set_current_track);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_track", godot::PROPERTY_HINT_RESOURCE_TYPE, "Track"), "set_current_track", "get_current_track");

        // current track pitch
        godot::ClassDB::bind_method(godot::D_METHOD("get_current_track_pitch"), &rhythm::AudioEngine2::get_current_track_pitch);
        godot::ClassDB::bind_method(godot::D_METHOD("set_current_track_pitch", "p_track_pitch"), &rhythm::AudioEngine2::set_current_track_pitch);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "current_track_pitch"), "set_current_track_pitch", "get_current_track_pitch");

        // click
        godot::ClassDB::bind_method(godot::D_METHOD("get_click"), &rhythm::AudioEngine2::get_click);
        godot::ClassDB::bind_method(godot::D_METHOD("set_click", "p_track"), &rhythm::AudioEngine2::set_click);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "click", godot::PROPERTY_HINT_RESOURCE_TYPE, "Audio"), "set_click", "get_click");
    }

public:
    /* GODOT OVERRIDES */
    
    void _exit_tree() override
    {
        for(ma_sound& sound : sounds) ma_sound_uninit(&sound);
        sounds.clear();

        ma_engine_uninit(&engine);
    }

    void _ready() override
    {
        // init miniaudio
        if( ma_engine_init(NULL, &engine) != MA_SUCCESS )
        {
            godot::print_error("[AudioEngine2::_ready] failed to initialize miniaudio engine!");
            return;
        }
        
        ma_engine_set_volume(&engine, volume);
        
        if(click.is_valid()) load_audio(click);
        else godot::print_line("[AudioEngine2::_ready] tried to load click Audio but one was not set. please set one in the inspector!");
        
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
        if(!current_track.is_valid() || !playing_track) return;
        if(!current_track.is_valid() || !current_track->loaded) return;
        if(playing_track && get_current_track_progress_in_frames() == get_current_track_length_in_frames())
        {
            godot::print_line("[AudioEngine2::_process] song ended!");
            pause_current_track();
            return;
        }
        
        /* track is playing */

        // process conductor
        conductor.process(ma_engine_get_time_in_pcm_frames(&engine));
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
            MA_SOUND_FLAG_ASYNC
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
        audio->AudioEngine_sounds_index = sounds.size()-1;
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
        if(current_track.is_valid() && current_track->loaded) pause_current_track();

        current_track = p_current_track;
        if(is_node_ready())
        {
            load_audio(current_track);
            
            int64_t global_current_frame = ma_engine_get_time_in_pcm_frames(&engine);
            conductor.seek(global_current_frame, ma_sound_get_time_in_pcm_frames(current_track->sound)); // using miniaudio read head ask seek position when setting new track, may or may not work!!!!!
            conductor.set_beats(global_current_frame, current_track->get_beats());
            
            set_current_track_pitch(current_track_pitch);
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
        conductor.seek(ma_engine_get_time_in_pcm_frames(&engine), frame); // again, this is a place where we are going off of the miniaudio read head for seeking ....
    }
    
    double get_current_track_progress() const { return static_cast<double>(get_current_track_progress_in_frames()) / static_cast<double>(get_current_track_length_in_frames()); }

}; // AudioEngine2

} // rhythm
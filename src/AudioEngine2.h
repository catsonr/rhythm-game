#pragma once

#include <list>

#include "miniaudio.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "Track.h"

namespace rhythm
{

/* CONDUCTOR */

struct Conductor
{
public:
    // TODO: separate next beat into next beat AND next beat to schedule
    int next_beat_index { 0 };
    int next_beat_to_schedule_index { 0 };
    double lookahead_window { 0.1 }; // what percentage of a second the Conductor will look ahead to schedule a beat
    int64_t LATENCY { 2000 }; // how many frames in advance beat playback is scheduled, in order to SOUND in time
    
    godot::Ref<rhythm::Track> track;
    godot::PackedInt64Array beats;

    // these values are used to calculate the absolute positions of a track's beats, and should be updated on:
    // track start, track pause, track seek, and pitch change
    // since these are all the operations that change when beats would occur (in global time, and in the case of pitch change: local as well)
    int64_t INITIAL_START_PAUSE_FRAME { -1 };
    int64_t GLOBAL_START_FRAME { INITIAL_START_PAUSE_FRAME };
    int64_t LOCAL_PAUSE_FRAME  { INITIAL_START_PAUSE_FRAME };
    
    
    
    void reset()
    {
        next_beat_index = 0;
        next_beat_to_schedule_index = 0;
        GLOBAL_START_FRAME = INITIAL_START_PAUSE_FRAME;
        LOCAL_PAUSE_FRAME  = INITIAL_START_PAUSE_FRAME;
    }
    
    void track_set(ma_engine* engine, const godot::Ref<rhythm::Track>& p_track, const float pitch, const bool track_is_playing)
    {
        track = p_track;
        beats = track->get_beats();
        
        godot::print_line("[Conductor::track_set] now following '", track->get_title(), "' w pitch @ ", pitch);
        
        // start Conductor from the top if the new track is starting from beginning
        if( ma_sound_get_time_in_pcm_frames(track->sound) == 0 ) reset();
        // otherwise seek to current frame
        else track_seeked(ma_engine_get_time_in_pcm_frames(engine), ma_sound_get_time_in_pcm_frames(track->sound), pitch, track_is_playing);
    }
    
    void pitch_set(const int64_t current_global_time, const int64_t current_local_time, const float pitch, const bool track_is_playing)
    {
        track_seeked(current_global_time, current_local_time, pitch, track_is_playing);
    }

    void process(ma_engine* engine, const float pitch, const godot::Ref<rhythm::Audio>& click)
    {
        if(GLOBAL_START_FRAME == INITIAL_START_PAUSE_FRAME) return; // track hasn't started!
        if(next_beat_index >= beats.size()) return; // no more beats left!
        
        // NEXT BEAT
        
        int64_t current_global_frame = ma_engine_get_time_in_pcm_frames(engine);
        int64_t current_local_frame = static_cast<int64_t>(ma_sound_get_time_in_pcm_frames(track->sound));
        
        int64_t next_beat_local_frame = beats[next_beat_index];
        int64_t next_beat_global_frame = GLOBAL_START_FRAME + next_beat_local_frame/pitch;
        
        if( current_global_frame >= next_beat_global_frame ) // just passed the "next beat"
        {
            next_beat_index++;
        }
        
        // NEXT BEAT TO SCHEDULE
        
        if(next_beat_to_schedule_index >= beats.size()) return; // no more beats to schedule!

        int64_t sample_rate = static_cast<int64_t>(ma_engine_get_sample_rate(engine)); // frames per second
        int64_t lookahead_window_frames = sample_rate * lookahead_window; // how many frames we will look ahead for a beat
        
        int64_t next_beat_to_schedule_local_frame = beats[next_beat_to_schedule_index];
        // to account for pitch, divide the local beat frame by pitch (equivalent to increasing speed of time by pitch)
        int64_t next_beat_to_schedule_global_frame = GLOBAL_START_FRAME + next_beat_to_schedule_local_frame/pitch;
        
        if( next_beat_to_schedule_global_frame - current_global_frame <= lookahead_window_frames ) // next beat is in lookahead window!
        {
            ma_sound_seek_to_pcm_frame(click->sound, 0);
            ma_sound_set_start_time_in_pcm_frames(click->sound, next_beat_to_schedule_global_frame - LATENCY);
            ma_sound_start(click->sound);
            
            next_beat_to_schedule_index++;
        }
    }
    
    void track_played(const int64_t at_global_frame, const float pitch)
    {
        // if track is played from beginning
        if( LOCAL_PAUSE_FRAME == INITIAL_START_PAUSE_FRAME )
        {
            GLOBAL_START_FRAME = at_global_frame;
            return;
        }
        
        // else track is being resumed
        GLOBAL_START_FRAME = at_global_frame - LOCAL_PAUSE_FRAME/pitch;
    }
    
    void track_paused(const int64_t at_local_frame)
    {
        LOCAL_PAUSE_FRAME = at_local_frame;
    }
    
    void track_seeked(const int64_t at_global_frame, const int64_t to_local_frame, const float pitch, const bool track_is_playing)
    {
        GLOBAL_START_FRAME = at_global_frame - to_local_frame/pitch;

        if(!track_is_playing) LOCAL_PAUSE_FRAME = to_local_frame;
        
        for(int i = 0; i < beats.size(); i++)
        {
            if(beats[i] <= to_local_frame) continue;
            
            next_beat_index = i;
            next_beat_to_schedule_index = i;
            break;
        }
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
        conductor.process(&engine, current_track_pitch, click);
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
            
            conductor.track_played(ma_engine_get_time_in_pcm_frames(&engine), current_track_pitch);
        } else godot::print_line("[AudioEngine2::play_current_track] nothing to do ...");
    }
    
    void pause_current_track()
    {
        if(current_track.is_valid() && current_track->loaded && playing_track)
        {
            ma_sound_stop(current_track->sound);
            playing_track = false;

            conductor.track_paused(ma_sound_get_time_in_pcm_frames(current_track->sound));
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
            conductor.track_set(&engine, current_track, current_track_pitch, playing_track);
            
            set_current_track_pitch(current_track_pitch);
        }
    }
    
    // current_track_pitch
    float get_current_track_pitch() const { return current_track_pitch; }
    void set_current_track_pitch(const float p_current_track_pitch)
    {
        /*
            this is still super buggy, although it seems to be working for what it is needed for:
            lerping pitch from 0.5 to 1.0 at Observatory start, and
            choosing arbitrary CONSTANT pitch during game
            
            the GLOBAL_START_FRAME recalculation logic was per gemini's request. it works well enough for now
        */
        
        float previous_pitch = current_track_pitch;
        current_track_pitch = p_current_track_pitch;

        if(is_node_ready() && current_track.is_valid())
        {
            int64_t current_global_frame = ma_engine_get_time_in_pcm_frames(&engine);
            int64_t current_local_frame = (current_global_frame - conductor.GLOBAL_START_FRAME)*previous_pitch;

            ma_sound_set_pitch(current_track->sound, current_track_pitch);

            if(playing_track) conductor.GLOBAL_START_FRAME = current_global_frame - current_local_frame/current_track_pitch;
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
    
    int64_t get_current_track_progress_in_frames() const
    {
        if(!current_track.is_valid() || !current_track->loaded) return 0;
        
        return (int64_t)ma_sound_get_time_in_pcm_frames(current_track->sound);
    }
    
    void set_current_track_progress_in_frames(int64_t frame)
    {
        if(!current_track.is_valid() || !current_track->loaded) return;

        frame = (frame > get_current_track_length_in_frames()) ? get_current_track_length_in_frames() : frame;
        
        ma_sound_seek_to_pcm_frame(current_track->sound, (ma_uint64)frame);
        conductor.track_seeked(ma_engine_get_time_in_pcm_frames(&engine), frame, current_track_pitch, playing_track);
    }
    
    double get_current_track_progress() const { return static_cast<double>(get_current_track_progress_in_frames()) / static_cast<double>(get_current_track_length_in_frames()); }

}; // AudioEngine2

} // rhythm
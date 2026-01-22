#pragma once

#include <list>

#include "miniaudio.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "Track.h"

namespace rhythm
{

struct AudioEngine2 : public godot::Node
{
    GDCLASS(AudioEngine2, Node)

private:
    ma_engine engine;
    std::list<ma_sound> sounds;
    
    float volume { 1.0 };
    
    godot::Ref<rhythm::Track> current_track;
    bool playing_track { false };
    float current_track_pitch { 1.0 }; 

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
        if( ma_engine_init(NULL, &engine) != MA_SUCCESS )
        {
            godot::print_error("[AudioEngine2::_ready] failed to initialize miniaudio engine!");
            return;
        }
        
        ma_engine_set_volume(&engine, volume);

        godot::print_line("[AudioEngine2::_ready] miniaudio initialized : )");
    }
    
    void _process(double delta) override
    {
        if(!playing_track) return;
        if(!current_track.is_valid() || !current_track->loaded) return;
        
        if(playing_track && get_current_track_progress_in_frames() == get_current_track_length_in_frames())
        {
            godot::print_line("[AudioEngine2::_process] song ended!");
            pause_current_track();
            return;
        }
        
        /* track is playing */

        // match miniaudio to our godot properties
        ma_engine_set_volume(&engine, volume);
        ma_sound_set_pitch(current_track->sound, current_track_pitch);
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
        
        godot::print_line("[AudioEngine2::load_sound] ", abs_path, " loaded as track ", (int)(sounds.size()-1), "!");
        
        return true;
    }
    
    bool load_audio(const godot::Ref<rhythm::Audio>& audio)
    {
        if( audio->loaded )
        {
            //godot::print_line("[AudioEngine::load_audio] ", audio->get_file_path(), " is already loaded! ignoring ...");
            return true;
        }
        if( !load_sound(audio->get_file_path()) )
        {
            godot::print_error("[AudioEngine::load_audio] failed to load audio '", audio->get_file_path(), "'!");
            return false;
        }
        
        audio->sound = &sounds.back();
        audio->AudioEngine_sounds_index = sounds.size()-1;
        audio->loaded = true;

        return true;
    }
    
    void play_current_track()
    {
        if(current_track->loaded && !playing_track)
        {
            ma_sound_start(current_track->sound);
            playing_track = true;
        } else godot::print_line("[AudioEngine2::play_current_track] nothing to do ...");
    }
    
    void pause_current_track()
    {
        if(current_track->loaded && playing_track)
        {
            ma_sound_stop(current_track->sound);
            playing_track = false;
        } else godot::print_line("[AudioEngine2::pause_current_track] nothing to do ...");
    }

    /* GETTERS & SETTERS */
    
    float get_volume() const { return volume; }
    void set_volume(const float p_volume) { volume = p_volume; if(is_node_ready()) ma_engine_set_volume(&engine, volume); }

    godot::Ref<rhythm::Track> get_current_track() const { return current_track; }
    void set_current_track(const godot::Ref<rhythm::Track>& p_current_track) { current_track = p_current_track; if(is_node_ready()) load_audio(current_track); }
    
    float get_current_track_pitch() const { return current_track_pitch; }
    void set_current_track_pitch(const float p_current_track_pitch) { current_track_pitch = p_current_track_pitch; }
    
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
    }
    
    double get_current_track_progress() const { return static_cast<double>(get_current_track_progress_in_frames()) / static_cast<double>(get_current_track_length_in_frames()); }

}; // AudioEngine2

} // rhythm
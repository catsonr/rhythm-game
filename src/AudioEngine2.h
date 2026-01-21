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
    
    godot::Ref<rhythm::Track> current_track;
    bool playing_track { false };
    float current_track_pitch { 1.0 }; 

protected:
    static void _bind_methods()
    {
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

        godot::print_line("[AudioEngine2::_ready] miniaudio initialized : )");
    }
    
    void _process(double delta) override
    {
        if(!playing_track) return;
        if(current_track.is_null() || !current_track->loaded) return;
        
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
        if(current_track->loaded) ma_sound_start(current_track->sound);
        
        playing_track = true;
    }
    
    void pause_current_track()
    {
        if(current_track->loaded) ma_sound_stop(current_track->sound);

        playing_track = false;
    }

    /* GETTERS & SETTERS */
    
    godot::Ref<rhythm::Track> get_current_track() const { return current_track; }
    void set_current_track(const godot::Ref<rhythm::Track>& p_current_track) { current_track = p_current_track; if(is_node_ready()) load_audio(current_track); }
    
    float get_current_track_pitch() const { return current_track_pitch; }
    void set_current_track_pitch(const float p_current_track_pitch) { current_track_pitch = p_current_track_pitch; }

}; // AudioEngine2

} // rhythm
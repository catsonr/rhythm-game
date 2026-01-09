#include "AudioEngine.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>

void rhythm::AudioEngine::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_volume"), &rhythm::AudioEngine::get_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("set_volume", "p_volume"), &rhythm::AudioEngine::set_volume);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "volume"), "set_volume", "get_volume");
    
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_track"), &rhythm::AudioEngine::get_current_track);
    godot::ClassDB::bind_method(godot::D_METHOD("set_current_track", "p_track"), &rhythm::AudioEngine::set_current_track);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_track", godot::PROPERTY_HINT_RESOURCE_TYPE, "Track"), "set_current_track", "get_current_track");
}

// a bit hacky, but seems to work
rhythm::AudioEngine* rhythm::AudioEngine::singleton = nullptr;
rhythm::AudioEngine::AudioEngine()
{
    if(singleton == nullptr) singleton = this;
    else godot::print_error("[AudioEngine::AudioEngine] another AudioEngine has been instantiated!");
}

void rhythm::AudioEngine::_ready()
{
    if( ma_engine_init(NULL, &engine) != MA_SUCCESS )
    {
        godot::print_error("[AudioEngine::_ready] failed to initialize miniaudio engine!");
        return;
    }
    
    if(current_track.is_valid()) load_sound(current_track->get_file_path());
}

void rhythm::AudioEngine::_process(double delta)
{

}

rhythm::AudioEngine* rhythm::AudioEngine::get_singleton()
{
    return singleton;
}

/**
 * loads a sound to be played
 * e.g. load_sound("res://audio/cool_song.mp3")
 * 
 * returns true on success and false otherwise
 */
bool rhythm::AudioEngine::load_sound(godot::String p_path)
{
    sounds.emplace_back();
    ma_sound& sound = sounds.back();
    
    const ma_uint32 flags = 
        MA_SOUND_FLAG_NO_SPATIALIZATION |
        MA_SOUND_FLAG_ASYNC
    ;

    godot::String abs_path = godot::ProjectSettings::get_singleton()->globalize_path(p_path.utf8().get_data());
    
    if( ma_sound_init_from_file(&engine, abs_path.utf8().get_data(), flags, NULL, NULL, &sound) != MA_SUCCESS )
    {
        godot::print_error("[AudioEngine::load_sound] unable to load ", abs_path, "!");
        return false;
    }
    
    ma_sound_start(&sound);
    godot::print_line("now playing: ", p_path);
    
    return true;
}

void rhythm::AudioEngine::set_volume(float p_volume)
{
    volume = p_volume;
    
    if(volume > 1) volume = 1;
    if(volume < 0) volume = 0;

    ma_engine_set_volume(&engine, volume);
}

float rhythm::AudioEngine::get_volume() const
{
    return volume;
}

void rhythm::AudioEngine::set_current_track(godot::Ref<rhythm::Track> p_track)
{
    current_track = p_track;
}

godot::Ref<rhythm::Track> rhythm::AudioEngine::get_current_track() const
{
    return current_track;
}
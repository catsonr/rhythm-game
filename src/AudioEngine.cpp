#include "AudioEngine.h"

#include <godot_cpp/classes/project_settings.hpp>

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
    
    ma_engine_set_volume(&engine, 0.5);
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
bool rhythm::AudioEngine::load_sound(const char* path)
{
    sounds.emplace_back();
    ma_sound& sound = sounds.back();
    
    const ma_uint32 flags = 
        MA_SOUND_FLAG_NO_SPATIALIZATION |
        MA_SOUND_FLAG_ASYNC
    ;

    godot::String abs_path = godot::ProjectSettings::get_singleton()->globalize_path(path);
    
    if( ma_sound_init_from_file(&engine, abs_path.utf8().get_data(), flags, NULL, NULL, &sound) != MA_SUCCESS )
    {
        godot::print_error("[AudioEngine::load_sound] unable to load ", abs_path, "!");
        return false;
    }
    
    ma_sound_start(&sound);
    godot::print_line("now playing: ", path);
    
    return true;
}
#include "AudioEngine.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>

void rhythm::AudioEngine::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_volume"), &rhythm::AudioEngine::get_volume);
    godot::ClassDB::bind_method(godot::D_METHOD("set_volume", "p_volume"), &rhythm::AudioEngine::set_volume);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "volume"), "set_volume", "get_volume");
    
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_track"), &rhythm::AudioEngine::get_current_track);
    godot::ClassDB::bind_method(godot::D_METHOD("set_current_track", "p_track"), &rhythm::AudioEngine::set_current_track);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "current_track", godot::PROPERTY_HINT_RESOURCE_TYPE, "Track"), "set_current_track", "get_current_track");
}

// a bit hacky, but seems to work (gemini's idea)
rhythm::AudioEngine* rhythm::AudioEngine::singleton = nullptr;
rhythm::AudioEngine::AudioEngine()
{
    if(singleton == nullptr) singleton = this;
    else godot::print_error("[AudioEngine::AudioEngine] another AudioEngine has been instantiated!");
}

rhythm::AudioEngine::~AudioEngine()
{
    // clean up miniaudio
    for(ma_sound sound : sounds) ma_sound_uninit(&sound);
    // ma_engine_uninit(&engine); // this crashes godot!
}

void rhythm::AudioEngine::_ready()
{
    if( ma_engine_init(NULL, &engine) != MA_SUCCESS )
    {
        godot::print_error("[AudioEngine::_ready] failed to initialize miniaudio engine!");
        return;
    }
    
    if(current_track.is_valid()) load_audio(current_track);
}

void rhythm::AudioEngine::_process(double delta)
{
    //godot::print_line(current_track->get_beats().size());
}

void rhythm::AudioEngine::_input(const godot::Ref<godot::InputEvent>& event)
{
    static godot::Input* input = godot::Input::get_singleton();
    static godot::PackedInt64Array beats;

    if(input->is_physical_key_pressed(godot::KEY_SPACE))
    {
        play_audio(current_track);
    }
    if(input->is_physical_key_pressed(godot::KEY_M))
    {
        ma_uint64 current_frame;
        ma_sound_get_cursor_in_pcm_frames(current_track->sound, &current_frame);
        
        beats.append(static_cast<int64_t>(current_frame));
        godot::print_line("beat ", beats.size(), " @ ", current_frame);
    }
    if(input->is_physical_key_pressed(godot::KEY_ENTER))
    {
        current_track->set_beats(beats);
        godot::ResourceSaver::get_singleton()->save(current_track);
        godot::print_line("sent ", beats.size(), " beats to current track");
    }
}

rhythm::AudioEngine* rhythm::AudioEngine::get_singleton()
{
    return singleton;
}

bool rhythm::AudioEngine::play_audio(const godot::Ref<rhythm::Audio>& audio)
{
    return ma_sound_start(audio->sound) == MA_SUCCESS;
}

/**
 * loads a sound to be played
 * e.g. load_sound("res://audio/cool_song.mp3")
 * 
 * returns true on success and false otherwise
 */
bool rhythm::AudioEngine::load_sound(const godot::String& p_path)
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

    return true;
}

/**
 * loads (initializes) a Track
 */
bool rhythm::AudioEngine::load_audio(const godot::Ref<rhythm::Audio>& audio)
{
    if( load_sound(audio->get_file_path()) )
    {
        audio->sound = &sounds.back();
        audio->AudioEngine_sounds_index = sounds.size() - 1;
        
        return true;
    }
    
    godot::print_error("[AudioEngine::load_audio] unable to load audio!");
    return false;
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

void rhythm::AudioEngine::set_current_track(const godot::Ref<rhythm::Track>& p_track)
{
    current_track = p_track;
}

godot::Ref<rhythm::Track> rhythm::AudioEngine::get_current_track() const
{
    return current_track;
}
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

    godot::ClassDB::bind_method(godot::D_METHOD("get_click"), &rhythm::AudioEngine::get_click);
    godot::ClassDB::bind_method(godot::D_METHOD("set_click", "p_track"), &rhythm::AudioEngine::set_click);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "click", godot::PROPERTY_HINT_RESOURCE_TYPE, "Audio"), "set_click", "get_click");

    godot::ClassDB::bind_method(godot::D_METHOD("get_CLICK_PLAYBACK_OFFSET"), &rhythm::AudioEngine::get_CLICK_PLAYBACK_OFFSET);
    godot::ClassDB::bind_method(godot::D_METHOD("set_CLICK_PLAYBACK_OFFSET", "p_CLICK_PLAYBACK_OFFSET"), &rhythm::AudioEngine::set_CLICK_PLAYBACK_OFFSET);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "CLICK_PLAYBACK_OFFSET"), "set_CLICK_PLAYBACK_OFFSET", "get_CLICK_PLAYBACK_OFFSET");
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
    if(click.is_valid()) load_audio(click);
    
    godot::print_line("[AudioEngine::_ready()] audio engine initialized\n\tusing CLICK_PLAYBACK_OFFSET = ", CLICK_PLAYBACK_OFFSET);
}

void rhythm::AudioEngine::_process(double delta)
{
    if(!playing_track) return;

    static int next_beat_index = 0;
    static const godot::PackedInt64Array& beats = current_track->get_beats(); // list of beats in local track time
    
    int64_t engine_time = static_cast<int64_t>(ma_engine_get_time_in_pcm_frames(&engine)); // global time
    
    if(next_beat_index < beats.size()) // if there are still beats to be scheduled 
    {
        int64_t next_beat_time = CURRENT_TRACK_START_FRAME + beats[next_beat_index]; // next beat position in global time
        
        const int64_t lookaheadwindow = 48000 * .1; // look ahead 1/10th of a second (making this the minimum distance required between beats)
        if(next_beat_time - engine_time <= lookaheadwindow)
        {
            // schedule a click at next beat position
            ma_sound_seek_to_pcm_frame(click->sound, 0);
            ma_sound_set_start_time_in_pcm_frames(click->sound, next_beat_time + CLICK_PLAYBACK_OFFSET);
            ma_sound_start(click->sound);
            
            godot::print_line("scheduled beat ", next_beat_index, " @ ", next_beat_time);
            next_beat_index++;
        }
        
    }
}

void rhythm::AudioEngine::_input(const godot::Ref<godot::InputEvent>& event)
{
    static godot::Input* input = godot::Input::get_singleton();
    static godot::PackedInt64Array beats;

    if(input->is_physical_key_pressed(godot::KEY_SPACE)) play_current_track();
    if(input->is_physical_key_pressed(godot::KEY_ESCAPE)) pause_current_track();
    if(input->is_physical_key_pressed(godot::KEY_BACKSLASH)) { ma_sound_start(click->sound); godot::print_line("[AudioEngine::_input] click"); }
    if(input->is_physical_key_pressed(godot::KEY_M))
    {
        int64_t engine_frame = static_cast<int64_t>(ma_engine_get_time_in_pcm_frames(&engine));
        int64_t current_frame = engine_frame - CURRENT_TRACK_START_FRAME;
        
        beats.append(current_frame);
        godot::print_line("beat ", beats.size(), " @ ", current_frame);
    }
    if(input->is_physical_key_pressed(godot::KEY_ENTER))
    {
        current_track->set_beats(beats);
        godot::ResourceSaver::get_singleton()->save(current_track);
        godot::print_line("sent ", beats.size(), " beats to current track");
    }
}

/**
 * loads a miniaudio sound to be played
 * not to be confused with Audio, which is the godot Resource wrapper for sounds
 * e.g. load_sound("res://audio/cool_song.mp3")
 * 
 * returns true on success and false otherwise
 */
bool rhythm::AudioEngine::load_sound(const godot::String& p_path)
{
    godot::print_line("[AudioEngine::load_sound] attempting to load '", p_path, "' ...");

    sounds.emplace_back();
    ma_sound& sound = sounds.back();
    
    const ma_uint32 flags = 
        MA_SOUND_FLAG_NO_SPATIALIZATION |
        MA_SOUND_FLAG_DECODE
        //MA_SOUND_FLAG_ASYNC
    ;

    godot::String abs_path = godot::ProjectSettings::get_singleton()->globalize_path(p_path.utf8().get_data());
    
    if( ma_sound_init_from_file(&engine, abs_path.utf8().get_data(), flags, NULL, NULL, &sound) != MA_SUCCESS )
    {
        godot::print_error("[AudioEngine::load_sound] unable to load ", abs_path, "!");
        return false;
    }
    
    godot::print_line("\t", p_path, " loaded as sound ", (int)(sounds.size() - 1), "!");

    return true;
}

/**
 * loads the attached audio file
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

/**
 * schedules the current track to play, returns the frame it will begin playing
 */
int64_t rhythm::AudioEngine::play_current_track(int64_t delay)
{
    if(playing_track)
    {
        godot::print_line("[AudioEngine::play_current_track] already playing, ignorning ...");
        return -1;
    }

    int64_t current_frame = static_cast<int64_t>(ma_engine_get_time_in_pcm_frames(&engine));
    
    int64_t current_track_start_frame = current_frame + delay;
    
    ma_sound_set_start_time_in_pcm_frames(current_track->sound, current_track_start_frame);
    ma_sound_start(current_track->sound);

    playing_track = true;
    if(CURRENT_TRACK_LOCAL_PAUSE_FRAME == INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME) // if playing from beginning not resuming
        CURRENT_TRACK_START_FRAME = current_track_start_frame;
    else
        CURRENT_TRACK_START_FRAME = current_track_start_frame - CURRENT_TRACK_LOCAL_PAUSE_FRAME; // move current track start time up as if it was never paused
    
    godot::print_line("[AudioEngine::play_current_track] scheduled @ ", current_track_start_frame);
    
    return current_track_start_frame;
}

void rhythm::AudioEngine::pause_current_track()
{
    if(!playing_track)
    {
        godot::print_line("[AudioEngine::pause_current_track] already paused, ignoring ...");
        return;
    }

    ma_sound_stop(current_track->sound);
    CURRENT_TRACK_LOCAL_PAUSE_FRAME = ma_engine_get_time_in_pcm_frames(&engine) - CURRENT_TRACK_START_FRAME;
    
    playing_track = false;
    
    godot::print_line("[AudioEngine::pause_current_track] paused @ local frame ", CURRENT_TRACK_LOCAL_PAUSE_FRAME);
}

void rhythm::AudioEngine::set_volume(float p_volume)
{
    volume = p_volume;
    
    if(volume > 1) volume = 1;
    if(volume < 0) volume = 0;

    ma_engine_set_volume(&engine, volume);
}
float rhythm::AudioEngine::get_volume() const { return volume; }

void rhythm::AudioEngine::set_current_track(const godot::Ref<rhythm::Track>& p_track) { current_track = p_track; }
godot::Ref<rhythm::Track> rhythm::AudioEngine::get_current_track() const { return current_track; }

void rhythm::AudioEngine::set_click(const godot::Ref<rhythm::Audio>& p_click) { click = p_click; }
godot::Ref<rhythm::Audio> rhythm::AudioEngine::get_click() const { return click; }

void rhythm::AudioEngine::set_CLICK_PLAYBACK_OFFSET(const int p_CLICK_PLAYBACK_OFFSET) { CLICK_PLAYBACK_OFFSET = p_CLICK_PLAYBACK_OFFSET; }
int rhythm::AudioEngine::get_CLICK_PLAYBACK_OFFSET() const { return CLICK_PLAYBACK_OFFSET; }
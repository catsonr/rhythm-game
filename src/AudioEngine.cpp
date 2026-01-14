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
    godot::ClassDB::bind_method(godot::D_METHOD("set_click", "p_click"), &rhythm::AudioEngine::set_click);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "click", godot::PROPERTY_HINT_RESOURCE_TYPE, "Audio"), "set_click", "get_click");

    godot::ClassDB::bind_method(godot::D_METHOD("get_click_up"), &rhythm::AudioEngine::get_click_up);
    godot::ClassDB::bind_method(godot::D_METHOD("set_click_up", "p_click_up"), &rhythm::AudioEngine::set_click_up);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "click_up", godot::PROPERTY_HINT_RESOURCE_TYPE, "Audio"), "set_click_up", "get_click_up");

    godot::ClassDB::bind_method(godot::D_METHOD("get_LATENCY"), &rhythm::AudioEngine::get_LATENCY);
    godot::ClassDB::bind_method(godot::D_METHOD("set_LATENCY", "p_LATENCY"), &rhythm::AudioEngine::set_LATENCY);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "LATENCY"), "set_LATENCY", "get_LATENCY");
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
    if(click_up.is_valid()) load_audio(click_up);
    
    godot::print_line("[AudioEngine::_ready] audio engine initialized\n\tusing LATENCY = ", LATENCY);
}

void rhythm::AudioEngine::_process(double delta)
{
    if(!playing_track) return;
    else if(get_current_track_progress_in_frames() == get_current_track_length_in_frames())
    {
        godot::print_line("[AudioEngine::_process] current track ended!");
        pause_current_track();
        return;
    }

    static const godot::PackedInt64Array& beats = current_track->get_beats(); // list of beats in TRUE local track time

    if(CURRENT_TRACK_NEXT_BEAT_INDEX < beats.size()) // if there are still beats
    {
        int64_t true_engine_frame = ma_engine_get_time_in_pcm_frames(&engine); // the global frame where miniaudio TRULY, currently is. NOT adjusted for LATENCY

        int64_t true_next_beat_frame = CURRENT_TRACK_START_FRAME + beats[CURRENT_TRACK_NEXT_BEAT_INDEX]; // the global frame at which the beat TRULY happens
        int64_t next_beat_frame = true_next_beat_frame - LATENCY; // the global frame at which the beat happens, adjusting for LATENCY
        
        int64_t LOOKAHEAD_WINDOW = 48000 * 0.01; // look ahead 1/100th of a second for next beat
        if(next_beat_frame - true_engine_frame <= LOOKAHEAD_WINDOW) // the next beat is in the look ahead window!
        {
            ma_sound_seek_to_pcm_frame(click->sound, 0);
            ma_sound_set_start_time_in_pcm_frames(click->sound, next_beat_frame); // schedule beat to play at ADJUSTED frame
            ma_sound_start(click->sound);
            godot::print_line("[AudioEngine::_process] scheduled beat ", CURRENT_TRACK_NEXT_BEAT_INDEX, " @ ", next_beat_frame);
            
            if(click_up.is_valid() && CURRENT_TRACK_NEXT_BEAT_INDEX + 1 < beats.size()) // if there is a beat after the next beat (to calculate upbeat), and if an upbeat sound is loaded
            {
                int64_t dt = (beats[CURRENT_TRACK_NEXT_BEAT_INDEX+1] - beats[CURRENT_TRACK_NEXT_BEAT_INDEX]) / 2;
                
                ma_sound_seek_to_pcm_frame(click_up->sound, 0);
                ma_sound_set_start_time_in_pcm_frames(click_up->sound, next_beat_frame + dt); // schedule beat to play at ADJUSTED frame
                ma_sound_start(click_up->sound);
            }

            CURRENT_TRACK_NEXT_BEAT_INDEX++; // the "next" beat is now the one after this
        }
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
    if(get_current_track_progress_in_frames() == get_current_track_length_in_frames() || playing_track) return -1;

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
    if(!playing_track) return;

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

int64_t rhythm::AudioEngine::get_current_track_length_in_frames() const
{
    ma_uint64 ma_track_length_in_frames;
    ma_sound_get_length_in_pcm_frames(current_track->sound, &ma_track_length_in_frames);
    
    return static_cast<int64_t>(ma_track_length_in_frames);
}

int64_t rhythm::AudioEngine::get_current_track_progress_in_frames() const
{
    if(CURRENT_TRACK_START_FRAME == INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME) return 0; // track not started yet (zero progress)
    
    if(!playing_track) return CURRENT_TRACK_LOCAL_PAUSE_FRAME; // track paused, so progress is where it was paused at

    int64_t progress = ma_engine_get_time_in_pcm_frames(&engine) - CURRENT_TRACK_START_FRAME;
    int64_t length = get_current_track_length_in_frames();
    
    return (progress > length) ? length : progress;
}

void rhythm::AudioEngine::set_current_track_progress_in_frames(int64_t frame)
{
    if(frame > get_current_track_length_in_frames()) set_current_track_progress_in_frames(get_current_track_length_in_frames());
    
    ma_sound_seek_to_pcm_frame(current_track->sound, frame);
    
    CURRENT_TRACK_START_FRAME = ma_engine_get_time_in_pcm_frames(&engine) - frame;
    if(!playing_track) CURRENT_TRACK_LOCAL_PAUSE_FRAME = frame;
    
    const godot::PackedInt64Array& beats = current_track->get_beats();
    for(int i = 0; i < beats.size(); i++)
    {
        if(beats[i] <= frame) continue;
        
        CURRENT_TRACK_NEXT_BEAT_INDEX = i;
        break;
    }
    
    godot::print_line("[AudioEngine::set_current_track_progress_in_frames] seeked to track frame ", frame, " with next beat = ", CURRENT_TRACK_NEXT_BEAT_INDEX);
}

godot::Ref<rhythm::Track> rhythm::AudioEngine::get_current_track() const { return current_track; }
void rhythm::AudioEngine::set_current_track(const godot::Ref<rhythm::Track>& p_track) { current_track = p_track; }

godot::Ref<rhythm::Audio> rhythm::AudioEngine::get_click() const { return click; }
void rhythm::AudioEngine::set_click(const godot::Ref<rhythm::Audio>& p_click) { click = p_click; }

godot::Ref<rhythm::Audio> rhythm::AudioEngine::get_click_up() const { return click_up; }
void rhythm::AudioEngine::set_click_up(const godot::Ref<rhythm::Audio>& p_click_up) { click_up = p_click_up; }

uint64_t rhythm::AudioEngine::get_LATENCY() const { return LATENCY; }
void rhythm::AudioEngine::set_LATENCY(const uint64_t p_LATENCY) { LATENCY = p_LATENCY; }
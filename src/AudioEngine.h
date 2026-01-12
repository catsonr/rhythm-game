#pragma once

#include "miniaudio.h"

#include <list>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>

#include "Track.h"

namespace rhythm
{

class AudioEngine : public godot::Node
{
    GDCLASS(AudioEngine, Node)

// these should be private ... but oh well :shrug:
public:
    ma_engine engine;
    std::list<ma_sound> sounds;
    float volume { 1.0 };

    godot::Ref<rhythm::Track> current_track;
    godot::Ref<rhythm::Audio> click; // plays on down beat
    godot::Ref<rhythm::Audio> click_up; // plays on upbeat
    
    const int64_t INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME { -1 };
    // the time at which the current track began playback, in global time
    int64_t CURRENT_TRACK_START_FRAME { INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME };
    // the frame at wich the current track was paused, in LOCAL time (i.e., where in the track it was paused)
    int64_t CURRENT_TRACK_LOCAL_PAUSE_FRAME { INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME };
    
    int CURRENT_TRACK_NEXT_BEAT_INDEX { 0 };

    /**
     * LATENCY is how long, in PCM frames, it takes for a sound in miniaudio to be heard by the user
     *
     * NOTE: this is an UNSIGNED integer! whereas most other frame variables are signed. this is because
     * you cannot hear a sound *before* it is played, and there will ALWAYS be some non-zero latency for any
     * one given setup
     * 
     * LATENCY is a global offset from the 'true' current frame
     */
    uint64_t LATENCY { 0 };

    bool playing_track { false };

protected:
    static void _bind_methods();

public:
    ~AudioEngine();
    
    /* GODOT OVERRIDES */

    void _ready() override;
    void _process(double delta) override;
    void _input(const godot::Ref<godot::InputEvent>& event) override;
    
    /* INTERNAL AUDIOENGINE API */
    
    bool load_sound(const godot::String& p_path);
    bool load_audio(const godot::Ref<rhythm::Audio>& audio);
    
    // Tracks must be started/stopped with these functions! do not use direct miniaudio calls!
    int64_t play_current_track(int64_t delay = 0);
    void pause_current_track();
    
    /* GETTERS & SETTERS */
    
    // volume
    void set_volume(float p_volume);
    float get_volume() const;

    int64_t get_current_track_length_in_frames() const;
    int64_t get_current_track_progress_in_frames() const;
    void set_current_track_progress_in_frames(int64_t frame);
    
    // current_track
    godot::Ref<rhythm::Track> get_current_track() const;
    void set_current_track(const godot::Ref<rhythm::Track>& p_track);

    // click
    godot::Ref<rhythm::Audio> get_click() const;
    void set_click(const godot::Ref<rhythm::Audio>& p_click);

    // click_up
    godot::Ref<rhythm::Audio> get_click_up() const;
    void set_click_up(const godot::Ref<rhythm::Audio>& p_click_up);
    
    // LATENCY
    uint64_t get_LATENCY() const;
    void set_LATENCY(const uint64_t p_LATENCY);
    
}; // AudioEngine

} // rhythm

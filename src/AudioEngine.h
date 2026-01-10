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

private:
    ma_engine engine;
    std::list<ma_sound> sounds;
    float volume { 1.0 };

    godot::Ref<rhythm::Track> current_track;
    godot::Ref<rhythm::Audio> click;
    
    // the time at which the current track began playback, in global time
    int64_t CURRENT_TRACK_START_FRAME { -1 };
    const int64_t INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME { -1 };
    // the local time at which the current track was paused
    int64_t CURRENT_TRACK_LOCAL_PAUSE_FRAME { INITIAL_CURRENT_TRACK_LOCAL_PAUSE_FRAME };
    // the number of frames to WAIT until click is played. positive values are later, negative values are earlier
    // this value will need to be calibrated per user ...
    int CLICK_PLAYBACK_OFFSET { -4200 };

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
    
    // current_track
    void set_current_track(const godot::Ref<rhythm::Track>& p_track);
    godot::Ref<rhythm::Track> get_current_track() const;

    // click
    void set_click(const godot::Ref<rhythm::Audio>& p_click);
    godot::Ref<rhythm::Audio> get_click() const;
    
    // CLICK_PLAYBACK_OFFSET
    void set_CLICK_PLAYBACK_OFFSET(const int p_CLICK_PLAYBACK_OFFSET);
    int get_CLICK_PLAYBACK_OFFSET() const;
}; // AudioEngine

} // rhythm

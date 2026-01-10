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
    static AudioEngine* singleton;
    ma_engine engine;
    std::list<ma_sound> sounds;
    float volume { 1.0 };

    godot::Ref<rhythm::Track> current_track;
    godot::Ref<rhythm::Audio> click;
    
    int64_t GLOBAL_TRACK_START_TIME { 48000 };
    // the number of frames to WAIT until click is played. positive values are later, negative values are earlier
    // this value will need to be calibrated per user ...
    int CLICK_PLAYBACK_OFFSET { -4200 }; 

protected:
    static void _bind_methods();

public:
    AudioEngine();
    ~AudioEngine();
    
    /* GODOT OVERRIDES */

    void _ready() override;
    void _process(double delta) override;
    void _input(const godot::Ref<godot::InputEvent>& event) override;
    
    /* INTERNAL AUDIOENGINE API */
    
    static AudioEngine* get_singleton();
    
    bool play_audio(const godot::Ref<rhythm::Audio>& audio);
    
    bool load_sound(const godot::String& p_path);
    bool load_audio(const godot::Ref<rhythm::Audio>& audio);
    
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

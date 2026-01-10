#pragma once

#include "miniaudio.h"

#include <vector>

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
    std::vector<ma_sound> sounds;
    float volume { 1.0 };

    godot::Ref<rhythm::Track> current_track;

protected:
    static void _bind_methods();

public:
    AudioEngine();
    ~AudioEngine();
    
    void _ready() override;
    void _process(double delta) override;
    void _input(const godot::Ref<godot::InputEvent>& event) override;
    
    static AudioEngine* get_singleton();
    
    bool play_audio(const godot::Ref<rhythm::Audio>& audio);
    
    bool load_sound(const godot::String& p_path);
    bool load_audio(const godot::Ref<rhythm::Audio>& audio);
    
    void set_volume(float p_volume);
    float get_volume() const;
    
    void set_current_track(const godot::Ref<rhythm::Track>& p_track);
    godot::Ref<rhythm::Track> get_current_track() const;
}; // AudioEngine

} // rhythm

#pragma once

#include "miniaudio.h"

#include <vector>
#include <godot_cpp/classes/node.hpp>

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
    
    void _ready() override;
    void _process(double delta) override;
    
    static AudioEngine* get_singleton();
    
    bool load_sound(godot::String p_path);
    
    void set_volume(float p_volume);
    float get_volume() const;
    
    void set_current_track(godot::Ref<rhythm::Track> p_track);
    godot::Ref<rhythm::Track> get_current_track() const;
}; // AudioEngine

} // rhythm

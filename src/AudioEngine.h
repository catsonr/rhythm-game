#pragma once

#include "miniaudio.h"

#include <vector>
#include <godot_cpp/classes/node.hpp>

namespace rhythm
{

class AudioEngine : public godot::Node
{
    GDCLASS(AudioEngine, Node)

private:
    static AudioEngine* singleton;
    ma_engine engine;
    std::vector<ma_sound> sounds;

protected:
    static void _bind_methods() {}

public:
    AudioEngine();
    
    void _ready() override;
    void _process(double delta) override;
    
    static AudioEngine* get_singleton();
    
    bool load_sound(const char* path);
}; // AudioEngine

} // rhythm

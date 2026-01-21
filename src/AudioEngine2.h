#pragma once

#include <list>

#include <godot_cpp/classes/node.hpp>

#include "miniaudio.h"

namespace rhythm
{

struct AudioEngine2 : public godot::Node
{
    GDCLASS(AudioEngine2, Node)

private:
    ma_engine engine;
    std::list<ma_sound> sounds;

protected:
    static void _bind_methods() {}

public:

}; // AudioEngine2

} // rhythm
#pragma once

#include <godot_cpp/classes/node.hpp>

class Minimal : public godot::Node
{
    GDCLASS(Minimal, Node)

protected:
    // godot requires this method to be defined
    static void _bind_methods() {}

public:
}; // Minimal
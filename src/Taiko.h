#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "AudioEngine.h"
#include "Track.h"

namespace rhythm
{

class Taiko : public godot::Control
{
    GDCLASS(Taiko, Control)

private:
    rhythm::AudioEngine* audio_engine { nullptr };

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine"), &rhythm::Taiko::get_audio_engine);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine", "p_audio_engine"), &rhythm::Taiko::set_audio_engine);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "audio_engine", godot::PROPERTY_HINT_NODE_TYPE, "AudioEngine"), "set_audio_engine", "get_audio_engine");
    }

public:
    void _ready() override
    {
        if( audio_engine == nullptr ) godot::print_error("[BeatGraph::_ready()] no AudioEngine linked! please set one in the inspector!");
    }
    
    void _process(double delta) override
    {
        queue_redraw();
    }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        
        godot::Rect2 background_rect { 0, 0, w, h };
        godot::Color background_color { 0.1, 0.1, 0.1, 1 };
        draw_rect(background_rect, background_color, true);
    }

    /* GETTERS & SETTERS */

    rhythm::AudioEngine* get_audio_engine() const { return audio_engine; }
    void set_audio_engine(rhythm::AudioEngine* p_audio_engine) { audio_engine = p_audio_engine; }
}; // Taiko

} // rhythm
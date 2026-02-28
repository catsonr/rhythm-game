#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

#include "DSPGraphEdit.h"

#include "nodes/sm/SceneMachine.h"

namespace rhythm::dsp
{

struct DSPGraphEditor : public godot::Control
{
    GDCLASS(DSPGraphEditor, godot::Control)

private:
    dsp::DSPGraphEdit* dspgraphedit;
    godot::Button* save_graph_button;

public:
    void _ready() override
    {
        dspgraphedit = memnew(dsp::DSPGraphEdit);
        dspgraphedit->set_anchors_and_offsets_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
        add_child(dspgraphedit);
        
        // make bc opaque (bc it's transparent by default ??)
        godot::Ref<godot::StyleBoxFlat> styleboxflat;
        styleboxflat.instantiate();
        styleboxflat->set_bg_color({ 0.1, 0.1, 0.1, 1 });
        dspgraphedit->add_theme_stylebox_override("panel", styleboxflat);
        
        save_graph_button = memnew(godot::Button);
        save_graph_button->set_text("save graph");
        save_graph_button->set_anchors_and_offsets_preset(godot::Control::LayoutPreset::PRESET_BOTTOM_LEFT);
        add_child(save_graph_button);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if(key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo())
        {
            switch(key_event->get_physical_keycode())
            {
                // track selection
                case godot::KEY_ESCAPE:
                {
                    godot::Ref<godot::PackedScene> title_screen_scene = godot::ResourceLoader::get_singleton()->load("res://scenes/title_screen.tscn");
                    if( title_screen_scene.is_valid() )
                    {
                        sm::SceneMachine* sm = sm::BXScene::get_machine(this);
                        if( sm ) sm->enter_scene(title_screen_scene);
                    }

                    break;
                }
                
                default: break;
            }
        }
    }

protected:
    static void _bind_methods() {}
}; // DSPGraphEditor

} // rhythm::dsp
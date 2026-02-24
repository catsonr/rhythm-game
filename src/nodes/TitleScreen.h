#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct TitleScreen : public godot::Control
{
    GDCLASS(TitleScreen, Control)

private:
    godot::ColorRect* bg_shader { nullptr };
    godot::Ref<godot::ShaderMaterial> bg_shader_material;

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_bg_shader_material"), &rhythm::TitleScreen::get_bg_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_bg_shader_material", "p_bg_shader_material"), &rhythm::TitleScreen::set_bg_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "bg_shader_material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_bg_shader_material", "get_bg_shader_material");
    }

public:
    void _ready() override
    {
        bg_shader = memnew(godot::ColorRect);
        bg_shader->set_name("bg_shader");
        bg_shader->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        bg_shader->set_draw_behind_parent(true);
        if( bg_shader_material.is_valid() ) bg_shader->set_material(bg_shader_material);
        else godot::print_error("[TitleScreen::_ready] invalid bg shader material. please set one in the inspector!");
        add_child(bg_shader);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case KEY_ENTER:
                {
                    godot::Ref<godot::PackedScene> observatory_scene = godot::ResourceLoader::get_singleton()->load("res://scenes/observatory.tscn");
                    if( observatory_scene.is_valid() ) Scene::conjure_ctx(this)->scene_manager->push_scene(observatory_scene, false);
                    else godot::print_error("[TitleScreen::_input] failed to load observatory scene ...");
                    
                    set_visible(false);
                    
                    break;
                }
                // really lazy way of getting into dsp graph, but it works for now
                // i guess the observatory one is lazy too...
                case KEY_D:
                {
                    
                    godot::Ref<godot::PackedScene> dsp_scene = godot::ResourceLoader::get_singleton()->load("res://scenes/dsp.tscn");
                    if( dsp_scene.is_valid() ) Scene::conjure_ctx(this)->scene_manager->push_scene(dsp_scene, false);
                    else godot::print_error("[TitleScreen::_input] failed to load dsp scene ...");
                    
                    set_visible(false);
                    
                    break;
                }
                
                default: break;
            }
        }
    }
    
    void _process(double delta) override
    {
        godot::Vector2 size = get_size();
        godot::Vector2 mouse = get_local_mouse_position();
        godot::Vector2 mouse_dist_from_center = (mouse - 0.5*size) / size;
        
        if( bg_shader_material.is_valid() ) bg_shader_material->set_shader_parameter("mouse_dist_from_center", mouse_dist_from_center);
    }

    godot::Ref<godot::ShaderMaterial> get_bg_shader_material() const { return bg_shader_material; }
    void set_bg_shader_material(const godot::Ref<godot::ShaderMaterial>& p_bg_shader_material) { bg_shader_material = p_bg_shader_material; }
}; // TitleScreen

} // rhythm

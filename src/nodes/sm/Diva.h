#pragma once

#include "nodes/sm/BXScene.h"

namespace rhythm::sm
{

struct Diva : public sm::BXScene
{
    GDCLASS(Diva, sm::BXScene)

    godot::ColorRect* background_shader;
    godot::Ref<godot::ShaderMaterial> background_shader_material;

public:
    
    godot::StringName bxname() const override { return "diva"; }

    void _ready() override
    {
        set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);

        if( !background_shader_material.is_valid() ) godot::print_error("[Diva::_ready] no background shader set! please set one in the inspector");
        background_shader = memnew(godot::ColorRect);
        background_shader->set_name("background_shader");
        background_shader->set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        background_shader->set_draw_behind_parent(true);
        if( background_shader_material.is_valid() ) background_shader->set_material(background_shader_material);
        add_child(background_shader);
    }

    /* GETTERS & SETTERS */
    
    godot::Ref<godot::ShaderMaterial> get_background_shader_material() const { return background_shader_material; }
    void set_background_shader_material(const godot::Ref<godot::ShaderMaterial>& p_background_shader_material) { background_shader_material = p_background_shader_material; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_background_shader_material"), &rhythm::sm::Diva::get_background_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_background_shader_material", "p_background_shader_material"), &rhythm::sm::Diva::set_background_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "background_shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_background_shader_material", "get_background_shader_material");
    }
}; // Diva

} // rhythm::sm

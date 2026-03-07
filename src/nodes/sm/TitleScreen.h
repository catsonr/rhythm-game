#pragma once

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "nodes/sm/SceneMachine.h"

namespace rhythm::sm
{

struct TitleScreen : public sm::BXScene
{
    GDCLASS(TitleScreen, sm::BXScene)

private:
    godot::ColorRect* bg_shader { nullptr };
    godot::Ref<godot::ShaderMaterial> bg_shader_material;

    godot::ColorRect* cross_texture_shader { nullptr };
    godot::Ref<godot::ShaderMaterial> cross_texture_shader_material;

    godot::ColorRect* bg_shader_bg { nullptr };
    
    double trans_t { 0.0 };

public:
    godot::StringName bxname() const override { return "title screen"; }

    void _ready() override
    {
        set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

        bg_shader = memnew(godot::ColorRect);
        bg_shader->set_name("bg_shader");
        bg_shader->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        if( bg_shader_material.is_valid() ) bg_shader->set_material(bg_shader_material);
        else godot::print_error("[TitleScreen::_ready] invalid bg shader material. please set one in the inspector!");
        add_child(bg_shader);

        cross_texture_shader = memnew(godot::ColorRect);
        cross_texture_shader->set_name("cross_texture_shader");
        cross_texture_shader->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        if( cross_texture_shader_material.is_valid() ) cross_texture_shader->set_material(cross_texture_shader_material);
        else godot::print_error("[TitleScreen::_ready] invalid cross_texture shader material. please set one in the inspector!");
        add_child(cross_texture_shader);
        
        bg_shader_bg = memnew(godot::ColorRect);
        bg_shader_bg->set_name("bg_shader_bg");
        bg_shader_bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
        bg_shader_bg->set_draw_behind_parent(true);
        add_child(bg_shader_bg);
    }
    
    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_ENTER:
                {
                    godot::Ref<godot::PackedScene> observatory_scene = godot::ResourceLoader::get_singleton()->load("res://scenes/observatory.tscn");
                    if( observatory_scene.is_valid() )
                    {
                        sm::SceneMachine* sm = sm::BXScene::get_machine(this);
                        if( sm )
                        {
                            sm->transition_scene(observatory_scene, true);
                            sm->trans.push_current_scene = true;
                            sm->trans.push_current_scene_input = false;
                        }
                    }
                    
                    break;
                }
                case godot::KEY_D:
                {
                    godot::Ref<godot::PackedScene> dsp_scene = godot::ResourceLoader::get_singleton()->load("res://scenes/dsp.tscn");
                    if( dsp_scene.is_valid() )
                    {
                        sm::SceneMachine* sm = sm::BXScene::get_machine(this);
                        if( sm ) sm->push_scene(dsp_scene);
                    }
                    
                    break;
                }
                
                default: break;
            }
        }
    }
    
    void _process(double delta) override
    {
        godot::Vector2 size = get_size();
        godot::Vector2 mouse = ( get_local_mouse_position() - 0.5*size ) / size;
        mouse.y = -mouse.y;
        
        if( !bg_shader_material.is_valid() ) return;

        bg_shader_material->set_shader_parameter("size", size);
        bg_shader_material->set_shader_parameter("mouse", mouse);
        bg_shader_material->set_shader_parameter("trans_t", trans_t);
        
        if( !cross_texture_shader_material.is_valid() ) return; 

        cross_texture_shader_material->set_shader_parameter("size", size);
        
        //godot::print_line("[TitleScreen::_process] trans_t=" + godot::String::num_real(trans_t));
    }
    
    void transition_in(const sm::Transition& trans) override
    {
        trans_t = trans.t_end - trans.t; // reverse for transitioning back in
    }
    
    void transition_out(const sm::Transition& trans) override
    {
        trans_t = trans.t; // forwards for transitioning out

        cross_texture_shader->set_visible(false);
        bg_shader_bg->set_visible(false);
        
    }

    godot::Ref<godot::ShaderMaterial> get_bg_shader_material() const { return bg_shader_material; }
    void set_bg_shader_material(const godot::Ref<godot::ShaderMaterial>& p_bg_shader_material) { bg_shader_material = p_bg_shader_material; }

    godot::Ref<godot::ShaderMaterial> get_cross_texture_shader_material() const { return cross_texture_shader_material; }
    void set_cross_texture_shader_material(const godot::Ref<godot::ShaderMaterial>& p_cross_texture_shader_material) { cross_texture_shader_material = p_cross_texture_shader_material; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_bg_shader_material"), &TitleScreen::get_bg_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_bg_shader_material", "p_bg_shader_material"), &TitleScreen::set_bg_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "bg_shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_bg_shader_material", "get_bg_shader_material");

        godot::ClassDB::bind_method(godot::D_METHOD("get_cross_texture_shader_material"), &TitleScreen::get_cross_texture_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_cross_texture_shader_material", "p_cross_texture_shader_material"), &TitleScreen::set_cross_texture_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cross_texture_shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_cross_texture_shader_material", "get_cross_texture_shader_material");
    }
}; // TitleScreen

} // rhythm::sm
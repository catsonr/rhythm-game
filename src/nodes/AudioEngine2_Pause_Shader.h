#pragma once

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include "AudioEngine2.h"

namespace rhythm
{

struct AudioEngine2_Pause_Shader : public::godot::ColorRect
{
    GDCLASS(AudioEngine2_Pause_Shader, ColorRect)

private:
    godot::NodePath audio_engine_2_path;
    godot::Ref<godot::ShaderMaterial> shader_material;
    AudioEngine2* audio_engine_2;

public:
    void _ready() override
    {
        // audio engine
        if( audio_engine_2_path.is_empty() ) { godot::print_error("[AudioEngine2_Pause_Shader::_ready] a NodePath to AudioEngine2 has not been set. please set one in the inspector!"); return; }
        
        godot::Node* node = get_node_or_null(audio_engine_2_path);
        audio_engine_2 = godot::Object::cast_to<rhythm::AudioEngine2>(node);
        
        if( !audio_engine_2 ) { godot::print_error("[AudioEngine2_Pause_Shader::_ready] a path to AudioEngine2 has been set, but is it not an AudioEngine2!"); return; }
        
        // shader material 
        if(shader_material.is_valid()) set_material(shader_material);
        else godot::print_error("[AudioEngine2_Pause_Shader] no shader material set. please set one in the inspector!");

        // color rect
        set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        set_mouse_filter(MouseFilter::MOUSE_FILTER_IGNORE);
    }
    
    void _process(double delta) override
    {
        shader_material->set_shader_parameter("size", get_size());
        shader_material->set_shader_parameter("playing_track", audio_engine_2->playing_track);
    }
    
    godot::NodePath get_audio_engine_2_path() const { return audio_engine_2_path; }
    void set_audio_engine_2_path(const godot::NodePath& p_audio_engine_2_path) { audio_engine_2_path = p_audio_engine_2_path; }
    
    godot::Ref<godot::ShaderMaterial> get_shader_material() const { return shader_material; }
    void set_shader_material(const godot::Ref<godot::ShaderMaterial> p_shader_material) { shader_material = p_shader_material; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_2_path"), &rhythm::AudioEngine2_Pause_Shader::get_audio_engine_2_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_2_path", "p_audio_engine_2_path"), &rhythm::AudioEngine2_Pause_Shader::set_audio_engine_2_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_2_path"), "set_audio_engine_2_path", "get_audio_engine_2_path");

        godot::ClassDB::bind_method(godot::D_METHOD("get_shader_material"), &rhythm::AudioEngine2_Pause_Shader::get_shader_material);
        godot::ClassDB::bind_method(godot::D_METHOD("set_shader_material", "p_shader_material"), &rhythm::AudioEngine2_Pause_Shader::set_shader_material);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "shader_material", godot::PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"), "set_shader_material", "get_shader_material");
    }
}; // AudioEngine2_Pause_Shader

} // rhythm

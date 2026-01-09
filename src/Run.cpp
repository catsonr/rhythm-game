#include "Run.h"
#include <godot_cpp/core/class_db.hpp>

void rhythm::Run::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_t"), &rhythm::Run::get_t);
    godot::ClassDB::bind_method(godot::D_METHOD("set_t", "p_t"), &rhythm::Run::set_t);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "t"), "set_t", "get_t");
}

rhythm::Run::Run()
{
}

void rhythm::Run::_ready()
{
    BG = get_node<godot::Sprite2D>("BG");
    if(!BG)
    {
        godot::print_error("[Run::_ready] could not find child node 'BG'!");
    }
    
    godot::Ref<godot::Material> BG_material = BG->get_material();
    
    if(BG_material.is_null())
    {
        godot::print_error("[Run::_ready] could not find material of child 'BG'!");
    }
    
    godot::ShaderMaterial* shadermaterial = godot::Object::cast_to<godot::ShaderMaterial>(BG_material.ptr());
    
    if(!shadermaterial)
    {
        godot::print_error("[Run::_ready] count not cast child 'BG's material to ShaderMaterial!");
    }
    
    BG_shadermaterial = godot::Ref<godot::ShaderMaterial>(shadermaterial);
}

void rhythm::Run::_process(double delta)
{
    t += delta;
    
    BG_shadermaterial->set_shader_parameter("t", t);
    
    if(!BG_shadermaterial.is_valid()) { godot::print_error("invalid!"); }
}

void rhythm::Run::set_t(double p_t)
{
    t = p_t;
}

double rhythm::Run::get_t() const
{
    return t;
}
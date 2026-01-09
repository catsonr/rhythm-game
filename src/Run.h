#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace rhythm
{

class Run : public godot::Node
{
    GDCLASS(Run, Node)

private:
    double t { 0 };
    
    godot::Sprite2D* BG { nullptr };
    godot::Ref<godot::ShaderMaterial> BG_shadermaterial { nullptr };

protected:
    // godot required method
    static void _bind_methods();

public:
    Run();

    // called every frame
    void _process(double delta) override;
    void _ready() override;
    
    // getters n setters
    void set_t(double p_t);
    double get_t() const;
}; // Run

} // rhythm

#include "gdexample.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDExample::_bind_methods()
{

}

GDExample::GDExample()
{
    time_passed = 0.0;
}

GDExample::~GDExample()
{

}

void GDExample::_process(double delta)
{
    time_passed += delta;
    
    Vector2 pos = Vector2( 100+50*cos(time_passed), 100+50*sin(time_passed) );
    set_position(pos);
}
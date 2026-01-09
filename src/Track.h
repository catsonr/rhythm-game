#pragma once

#include <vector>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace rhythm
{

class Track : public godot::Resource
{
    GDCLASS(Track, Resource);

private:
    godot::String file_path;
    std::vector<u_int64_t> beats;

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_file_path"), &rhythm::Track::get_file_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_file_path", "p_file_path"), &rhythm::Track::set_file_path);
        
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "file_path"), "set_file_path", "get_file_path");
    }

public:
    godot::String get_file_path() const { return file_path; }
    void set_file_path(godot::String p_file_path) { file_path = p_file_path; }
}; // Track

} // rhythm
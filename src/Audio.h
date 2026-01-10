#pragma once

#include "miniaudio.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace rhythm
{

class Audio : public godot::Resource
{
    GDCLASS(Audio, Resource);

public:
    // these should probably(?) be private, however it makes it much easier to load a Track if AudioEngine can directly modify it
    ma_sound* sound;
    int AudioEngine_sounds_index;
private:
    godot::StringName file_path;

protected:
    static void _bind_methods()
    {
        // file_path
        godot::ClassDB::bind_method(godot::D_METHOD("get_file_path"), &rhythm::Audio::get_file_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_file_path", "p_file_path"), &rhythm::Audio::set_file_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "file_path"), "set_file_path", "get_file_path");
    }

public:
    // file_path
    godot::StringName get_file_path() const { return file_path; }
    void set_file_path(const godot::StringName& p_file_path) { file_path = p_file_path; }
    
}; // Audio

} // rhythm
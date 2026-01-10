#pragma once

#include <vector>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace rhythm
{

class Track : public godot::Resource
{
    GDCLASS(Track, Resource);

public:
    // these should probably(?) be private, however it makes it much easier to load a Track if AudioEngine can directly modify it
    ma_sound* sound;
    int AudioEngine_sounds_index;
private:
    godot::String file_path; // TODO: make this StringName
    godot::PackedInt64Array beats; // PackedInt64Arrays are signed (and miniaudio PCM frames are UNsigned), but (2^63-1)/48000/60/60/24/365/100=60,931 centuries of audio, so just keep it signed

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_file_path"), &rhythm::Track::get_file_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_file_path", "p_file_path"), &rhythm::Track::set_file_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "file_path"), "set_file_path", "get_file_path");
        
        godot::ClassDB::bind_method(godot::D_METHOD("get_beats"), &rhythm::Track::get_beats);
        godot::ClassDB::bind_method(godot::D_METHOD("set_beats", "p_beats"), &rhythm::Track::set_beats);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_INT64_ARRAY, "beats"), "set_beats", "get_beats");
    }

public:
    godot::String get_file_path() const { return file_path; }
    void set_file_path(const godot::String& p_file_path) { file_path = p_file_path; }
    
    godot::PackedInt64Array get_beats() const { return beats; }
    void set_beats(const godot::PackedInt64Array& p_beats) { beats = p_beats; }
}; // Track

} // rhythm
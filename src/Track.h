#pragma once

#include "Audio.h"

namespace rhythm
{

class Track : public Audio
{
    GDCLASS(Track, Audio);

private:
    godot::PackedInt64Array beats; // PackedInt64Arrays are signed (and miniaudio PCM frames are UNsigned), but (2^63-1)/48000/60/60/24/365/100=60,931 centuries of audio, so just keep it signed

protected:
    static void _bind_methods()
    {
        // beats
        godot::ClassDB::bind_method(godot::D_METHOD("get_beats"), &rhythm::Track::get_beats);
        godot::ClassDB::bind_method(godot::D_METHOD("set_beats", "p_beats"), &rhythm::Track::set_beats);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_INT64_ARRAY, "beats"), "set_beats", "get_beats");
    }

public:
    // beats
    godot::PackedInt64Array get_beats() const { return beats; }
    void set_beats(const godot::PackedInt64Array& p_beats) { beats = p_beats; }
}; // Track

} // rhythm
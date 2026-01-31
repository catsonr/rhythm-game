#pragma once

#include <vector>

#include "Album.h"
#include "Audio.h"

namespace rhythm
{

class Track : public Audio
{
    GDCLASS(Track, Audio);

private:
    godot::Ref<rhythm::Album> album;
    godot::StringName title;

    // beats is an int64_t array where the value at each index represents what PCM frame of a sound that beat lies on
    // PackedInt64Arrays are signed (and miniaudio PCM frames are UNsigned), but it would take
    // (2^63-1)/48000/60/60/24/365/100 = 60,931 centuries of audio to overflow
    godot::PackedInt64Array beats; 
    
    // notes_packed is an int64_t array, where each Note struct is packed/serialized as:
    // 0 x FFFFFFFF FFFF      FFFF
    //     beat     numerator type
    // thus the order is chronological, i.e., later notes are greater than earlier ones
    //    this rule does break for notes that exist at the same time. though in that case, their difference in value is not significant 
    godot::PackedInt64Array notes_packed;
    // pulses per quarter (note)
    // this can be any value, though highly compisite numbers are obviously preferred
    // 2520 has been chosen since it is the smallest integer with 1, 2, 3, 4, 5, 6, 7, 8, 9, and 10 as its divisors
    // meaning we can represent exact n-tuplets, for any 1-10 n :)
public:
    static const int PPQ = 2520;

    struct Note
    {
        uint32_t beat; // which beat the note lies on
        uint16_t numerator; // the numerator of numerator/PPQ, represents *where* on the beat the note lies
        uint16_t type; // the type of note

        double position; // the result of numerator/PPQ
        
        enum Type : uint16_t
        {
            RIGHT = 0,
            LEFT  = 1
        }; // Type
        
        explicit Note(uint64_t packed_value) :
            beat(packed_value >> 32),
            numerator((packed_value >> 16) & 0xFFFF),
            position(static_cast<double>(numerator) / static_cast<double>(PPQ)),
            type(packed_value & 0xFFFF)
        {}
        
        Note(uint32_t beat, uint16_t numerator, uint16_t type) :
            beat(beat),
            numerator(numerator),
            position(static_cast<double>(numerator) / static_cast<double>(PPQ)),
            type(type)
        {}
        
        // returns the packed int value
        uint64_t packed_value() const { return ((uint64_t)beat << 32) | ((uint64_t)numerator << 16) | ((uint64_t)type); }
        
        std::vector<Note> static unpack(const godot::PackedInt64Array& array)
        {
            std::vector<Note> notes;

            for(uint64_t packed_value : array) notes.emplace_back(packed_value);
            
            return notes;
        }
        
        godot::PackedInt64Array static pack(const std::vector<Note> notes)
        {
            godot::PackedInt64Array array;

            for(const Note& note : notes) array.append(note.packed_value());
            
            return array;
        }
        
        uint16_t static position_to_numerator(double position) { return PPQ*position; }
    }; // Note

protected:
    static void _bind_methods()
    {
        // album
        godot::ClassDB::bind_method(godot::D_METHOD("get_album"), &rhythm::Track::get_album);
        godot::ClassDB::bind_method(godot::D_METHOD("set_album", "p_album"), &rhythm::Track::set_album);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "album", godot::PROPERTY_HINT_RESOURCE_TYPE, "Album"), "set_album", "get_album");
        
        // title
        godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &rhythm::Track::get_title);
        godot::ClassDB::bind_method(godot::D_METHOD("set_title", "p_title"), &rhythm::Track::set_title);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "title"), "set_title", "get_title");

        // beats
        godot::ClassDB::bind_method(godot::D_METHOD("get_beats"), &rhythm::Track::get_beats);
        godot::ClassDB::bind_method(godot::D_METHOD("set_beats", "p_beats"), &rhythm::Track::set_beats);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_INT64_ARRAY, "beats"), "set_beats", "get_beats");

        // notes_packed
        godot::ClassDB::bind_method(godot::D_METHOD("get_notes_packed"), &rhythm::Track::get_notes_packed);
        godot::ClassDB::bind_method(godot::D_METHOD("set_notes_packed", "p_notes_packed"), &rhythm::Track::set_notes_packed);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::PACKED_INT64_ARRAY, "notes_packed"), "set_notes_packed", "get_notes_packed");
    }

public:

    /* beats (PackedInt64Array) opeations */

    static godot::PackedInt64Array delete_beat_at_index(const godot::PackedInt64Array& beats, int i)
    {
        if( i < 0 || i >= beats.size() )
        {
            godot::print_error("[Track::delete_beat_at_index] attempted to delete a beat (", i, ") at an out of bounds index!");
            return beats;
        }
        
        godot::PackedInt64Array new_beats = beats;
        new_beats.remove_at(i);
        
        return new_beats;
    }
    
    static constexpr int64_t minimum_recommended_beat_distance { 10000 };
    static godot::PackedInt64Array nudge_beat_at_index(const godot::PackedInt64Array& beats, int i, int64_t dframes)
    {
        if( i < 0 || i >= beats.size() )
        {
            godot::print_error("[Track::delete_beat_at_index] attempted to nudge a beat (", i, ") at an out of bounds index!");
            return beats;
        }
        
        int64_t new_frame = beats[i] + dframes;
        if(new_frame < 0) new_frame = 0;
        
        
        if( dframes >= 0 ) // nudge right (and identity)
        {
            if( i+1 < beats.size() && beats[i+1] - new_frame < minimum_recommended_beat_distance )
            {
                new_frame = beats[i+1] - minimum_recommended_beat_distance;
                godot::print_line("[Track::nudge_beat_at_index] attempted to nudege a beat (", i, ") too close to the next beat! using a minimum recommended beat distance of ", minimum_recommended_beat_distance, " instead");
            }
        }
        else // nudge left
        {
            if( i-1 >= 0 && new_frame - beats[i-1] < minimum_recommended_beat_distance )
            {
                new_frame = beats[i-1] - minimum_recommended_beat_distance;
                godot::print_line("[Track::nudge_beat_at_index] attempted to nudege a beat (", i, ") too close to the previous beat! using a minimum recommended beat distance of ", minimum_recommended_beat_distance, " instead");
            }
        }
        
        godot::PackedInt64Array new_beats = beats;
        new_beats[i] = new_frame;

        return new_beats;
    }

    /* GETTERS & SETTERS */

    // album
    godot::Ref<rhythm::Album> get_album() const { return album; }
    void set_album(const godot::Ref<rhythm::Album>& p_album) { album = p_album; }
    
    // title
    godot::StringName get_title() const { return title; }
    void set_title(const godot::StringName& p_title) { title = p_title; }

    // beats
    godot::PackedInt64Array get_beats() const { return beats; }
    void set_beats(const godot::PackedInt64Array& p_beats) { beats = p_beats; }

    // notes_packed
    godot::PackedInt64Array get_notes_packed() const { return notes_packed; }
    void set_notes_packed(const godot::PackedInt64Array& p_notes_packed) { notes_packed = p_notes_packed; }
}; // Track

} // rhythm
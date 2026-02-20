#pragma once

#include <vector>
#include <bitset>

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
    // 0 x FFFFFFFF 0FFF      FF   FF
    //     beat     numerator type modifier
    // thus the order is chronological, i.e., later notes are greater than earlier ones
    //    this rule does break for notes that exist at the same time. though in that case, their difference in value is not significant 
    godot::PackedInt64Array notes_packed;

public:
    struct Note
    {
        // pulses per quarter (note)
        // this can be any value, though highly compisite numbers are obviously preferred
        // 2520 has been chosen since it is the smallest integer with 1, 2, 3, 4, 5, 6, 7, 8, 9, and 10 as its divisors
        // meaning we can represent exact n-tuplets, for any 1-10 n :)
        static constexpr uint16_t PPQ = 2520;

        uint32_t beat; // which beat the note lies on
        uint16_t numerator : 12; // the numerator of numerator/PPQ, represents *where* on the beat the note lies

        // NOTE: numerator is 16 bits wide. however, with PPQ=2520, we only need ceil( log_2(2520) ) = 12 bits
        // thus the remaining four are currently unused
        // WARN: their use will cause undefined behavior since it will change packed_value() 
        uint16_t _reserved : 4;

        uint8_t type; // the type of note
        uint8_t modifier; // optional modifiers to the type, e.g., a hold note (Modifer::HOLD)


        enum Type : uint8_t
        {
            R1 = 0x00,
            R2 = 0x01,
            R3 = 0x02,
            R4 = 0x03,

            L1 = 0x10,
            L2 = 0x11,
            L3 = 0x12,
            L4 = 0x13
        }; // Type
        
        enum Modifier : uint8_t // 8 possible flags
        {
            NONE = 0b00000000,
            HOLD = 0b00000001
        }; // Modifer
        
        enum Mask : uint64_t
        {
            //                    | <-- unused!
            BEAT      = 0xFFFFFFFF00000000,
            NUMERATOR = 0x000000000FFF0000,
            TYPE      = 0x000000000000FF00,
            MODIFIER  = 0x00000000000000FF
        }; // Mask
        
        explicit Note(uint64_t packed_value) :
            beat(packed_value >> 32),
            numerator((packed_value >> 16) & 0x0FFF),
            _reserved(0),
            type((packed_value >> 8) & 0xFF),
            modifier(packed_value & 0xFF)
        {}
        
        Note(uint32_t beat, uint16_t numerator, uint8_t type, uint8_t modifier) :
            beat(beat),
            numerator(numerator),
            _reserved(0),
            type(type),
            modifier(modifier)
        {}
        
        /* Note operator overloads */

        bool operator==(const Note& other) const { return packed_value() == other.packed_value(); }
        bool operator<(const Note& other) const { return packed_value() < other.packed_value(); }

        /* Note lemmas */
        
        bool at_same_time(const Note& other) const
        {
            constexpr uint64_t TIME_MASK = Mask::BEAT | Mask::NUMERATOR;
            return (packed_value() & TIME_MASK) == (other.packed_value() & TIME_MASK);
        }
        
        // returns the result of numerator/PPQ, should be a value from [0.0, 1.0]
        double get_position() const { return static_cast<double>(numerator) / PPQ; }

        // performs the inverse of get_position(), i.e., converts a position to its closest numerator
        // TODO: test if this returns good values for all n-tuplets
        static uint16_t position_to_numerator(double position) { return std::round( PPQ*position ); }
        
        // returns the packed int value
        uint64_t packed_value() const { return ((uint64_t)beat << 32) | ((uint64_t)numerator << 16) | ((uint64_t)type << 8) | ((uint64_t)modifier); }
        
        /* notes (std::vector) lemmas */

        std::vector<Note> static unpack(const godot::PackedInt64Array& array)
        {
            std::vector<Note> notes;

            for(uint64_t packed_value : array) notes.emplace_back(packed_value);
            
            return notes;
        }
        
        godot::PackedInt64Array static pack(const std::vector<Note>& notes)
        {
            godot::PackedInt64Array array;

            for(const Note& note : notes) array.append(note.packed_value());
            
            return array;
        }
        
        // returns the index of a note 'key' in notes if it exists -- return -1 otherwise
        // this finds the exact match, including modifiers!
        // TODO: optional ignore modifiers
        static int32_t find_index(const std::vector<Note>& notes, const Note& key)
        {
            auto it = std::lower_bound(notes.begin(), notes.end(), key);
            
            if( it != notes.end() && *it == key ) return std::distance(notes.begin(), it);
            
            return -1;
        }
    
        static bool has_notes_at_beat_position(const std::vector<Note>& notes, uint32_t beat, double position)
        {
            Note key { beat, Note::position_to_numerator(position), 0, 0 };
            
            auto it = std::lower_bound(notes.begin(), notes.end(), key);
            
            return it != notes.end() && it->at_same_time(key);
        }
        
        static bool has_note(const std::vector<Note>& notes, const Note& key)
        {
            auto it = std::lower_bound(notes.begin(), notes.end(), key);
            
            return it != notes.end() && it->at_same_time(key) && it->type == key.type;
        }
        
        //std::bitset<256> note_types(const std::vector<Note>& notes);
        
        /* notes (std::vector) operations */
        
        static std::vector<Note> remove_note(const std::vector<Note>& notes, const Note& key)
        {
            if( !has_note(notes, key) )
            {
                godot::print_line("[Track::Note::remove_note] no Note (type ", key.type, ") exists at ", key.beat, ":", key.get_position(), "! ignoring ...");
                return notes;
            }

            int32_t beat_index = find_index(notes, key);
            if( beat_index == -1 )
            {
                godot::print_error("[Track::Note::remove_note] tried to find Note (type ", key.type, ") @ ", key.beat, ":", key.get_position(), ", but Track::Note::find_index() returned -1! ignoring ...");
                return notes;
            }
            
            std::vector<Note> new_notes = notes;
            new_notes.erase(new_notes.begin() + beat_index);
            
            return new_notes;
        }
        
        static std::vector<Note> add_note(const std::vector<Note>& notes, const Note& key)
        {
            if( has_note(notes, key) )
            {
                godot::print_line("[Track::Note::remove_note] Note (type ", key.type, ") already exists at ", key.beat, ":", key.get_position(), "! ignoring ...");
                return notes;
            }
            
            std::vector<Note> new_notes = notes;
            auto it = std::upper_bound(new_notes.begin(), new_notes.end(), key);
            new_notes.insert(it, key);
            
            return new_notes;
        }
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
    
    static godot::PackedInt64Array insert_beat_at_frame(const godot::PackedInt64Array& beats, int64_t local_frame)
    {
        const int64_t* start = beats.ptr();
        const int64_t* end   = start + beats.size();

        const int64_t* it = std::upper_bound(start, end, local_frame);

        const int next_beat_index = static_cast<int>( it-start );
        
        if(next_beat_index - 1 >= 0 && local_frame - beats[next_beat_index-1] < minimum_recommended_beat_distance)
        {
            godot::print_line("[Track::insert_beat_at_frame] attempted to insert a beat @ frame ", local_frame, " however there exists a previous beat at index ", next_beat_index-1, " that is below the minimum recommended beat distance. skipping beat insert ...");
            return beats;
        }
        if(next_beat_index < beats.size() && beats[next_beat_index] - local_frame < minimum_recommended_beat_distance)
        {
            godot::print_line("[Track::insert_beat_at_frame] attempted to insert a beat @ frame ", local_frame, " however there exists a next beat at index ", next_beat_index, " that is below the minimum recommended beat distance. skipping beat insert ...");
            return beats;
        }
        
        godot::PackedInt64Array new_beats = beats;
        new_beats.insert(next_beat_index, local_frame);
        
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
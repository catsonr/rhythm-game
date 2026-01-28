#pragma once

#include <stdint.h>
#include <algorithm>

#include <godot_cpp/classes/node.hpp>

#include "Audio.h"

namespace rhythm
{

struct Conductor
{
    /* STATE */

    static constexpr int64_t initial { -1 };
    int64_t global_start_frame { initial };
    int64_t local_pause_frame  { initial };
    double pitch { 1.0 };
    
    godot::PackedInt64Array beats;
    int next_beat_index { 0 };
    
    /* LEMMAS (known values that do not modify state) */

    bool is_playing() const { return global_start_frame != initial; }
    bool is_paused()  const { return local_pause_frame  != initial; }
    int64_t pause_frame() const { return (local_pause_frame != initial) ? local_pause_frame : 0; }
    int64_t get_local_current_frame(const int64_t global_current_frame) const
    {
        // if track is playing, the local current frame is elapsed frames times pitch
        if( is_playing() ) return static_cast<int64_t>( (global_current_frame - global_start_frame)*pitch );
        // otherwise we're paused, where we already know the frame we paused on (or we're at initial, which is frame 0)
        return pause_frame();
    }
    int next_beat_search(int64_t local_frame) const
    {
        const int64_t* start = beats.ptr();
        const int64_t* end   = start + beats.size();

        const int64_t* it = std::upper_bound(start, end, local_frame);
        return static_cast<int>( it-start );
    }
    
    /* OPERATIONS (actions that change state) */
    
    void set_beats(const int64_t global_current_frame, const godot::PackedInt64Array& p_beats)
    {
        beats = p_beats;
        next_beat_index = next_beat_search( get_local_current_frame(global_current_frame) );
    }
    
    void process(const int64_t global_current_frame)
    {
        if( beats.is_empty() || next_beat_index >= beats.size() || is_paused() ) return;
        
        int64_t local_current_time = get_local_current_frame(global_current_frame);
        while( next_beat_index < beats.size() && local_current_time >= beats[next_beat_index] )
        {
            godot::print_line("[Conductor::process] beat ", next_beat_index, "!");

            next_beat_index++;
        }
    }
    
    void play(const int64_t global_current_frame)
    {
        if( is_playing() ) return;

        global_start_frame = global_current_frame - static_cast<int64_t>( pause_frame()/pitch );
        // playing always sets local pause frame back to initial
        local_pause_frame = initial;
    }
    
    void pause(const int64_t global_current_frame)
    {
        if( is_paused() ) return;

        local_pause_frame = get_local_current_frame(global_current_frame);
        // pausing always sets global start frame back to initial
        global_start_frame = initial;
    }
    
    void seek(const int64_t global_current_frame, const int64_t to_local_frame)
    {
        if( is_paused() ) local_pause_frame = to_local_frame;
        else global_start_frame = global_current_frame - static_cast<int64_t>( to_local_frame/pitch );
        
        next_beat_index = next_beat_search(to_local_frame);
    }
    
    void set_pitch(const int64_t global_current_frame, const double p_pitch)
    {
        if( pitch == p_pitch ) return;

        if( is_playing() ) global_start_frame = global_current_frame - static_cast<int64_t>( get_local_current_frame(global_current_frame)/p_pitch );
        
        pitch = p_pitch;
    }
}; // Conductor

} // rhythm

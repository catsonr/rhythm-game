#pragma once

/*
    Constellations are BEATBOXX playlists
*/

#include <random>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include "Track.h"

namespace rhythm
{

struct Constellation : public godot::Resource
{
    GDCLASS(Constellation, Resource)

public:
    /*
      0  1  2
      TL TC TR
    7 ML xx MR 3
      BL BC BR 
      6  5  4

      thus for any direction, opposite = (forwards+4) % 8 aka (^ 0x4)
      where forwards is { 0, 1, 2, 3 } and backwards is { 4, 5, 6, 7 }
    */
    enum Adjacency : uint8_t
    {
        // forwards
        TL = 1 << 0,
        TC = 1 << 1,
        TR = 1 << 2,
        MR = 1 << 3,
        // backwards (opposite of forwards)
        BR = 1 << 4,
        BC = 1 << 5,
        BL = 1 << 6,
        ML = 1 << 7
    };

    godot::TypedArray<Track> tracks;
    std::vector<godot::Ref<godot::Texture2D>> covers;
    std::vector<godot::Vector2i> ids; // can also be thought of as position
    std::vector<uint8_t> adjacencies;
    /*
        this texture, along with the adjacencies shader is currently unused
        for now, adjacencies are simply rendered with draw_line, in _draw()

        adjacency shader is still running however, and draws a rainbow circle at (0, 0), just to
        show that it is in fact running
    */
    godot::Ref<godot::ImageTexture> observatory_adjacency_shader_texture;
    
    int seed { 0 };

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_tracks"), &rhythm::Constellation::get_tracks);
        godot::ClassDB::bind_method(godot::D_METHOD("set_tracks", "p_track"), &rhythm::Constellation::set_tracks);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "tracks", godot::PROPERTY_HINT_TYPE_STRING, godot::String::num(godot::Variant::OBJECT) + "/" + godot::String::num(godot::PROPERTY_HINT_RESOURCE_TYPE) + ":Track"), "set_tracks", "get_tracks");

        godot::ClassDB::bind_method(godot::D_METHOD("get_seed"), &rhythm::Constellation::get_seed);
        godot::ClassDB::bind_method(godot::D_METHOD("set_seed", "p_seed"), &rhythm::Constellation::set_seed);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "seed"), "set_seed", "get_seed");
    }

public:
    // caches the cover, and generates the adjacencies (and thus positions) of each Track in tracks (since we assume first track is at (0, 0))
    void cache()
    {
        int tracks_size = tracks.size();

        covers.clear();
        covers.reserve(tracks_size);
        ids.clear();
        ids.reserve(tracks_size);
        adjacencies.clear();
        adjacencies.reserve(tracks_size);

        std::random_device random_device;
        std::mt19937 device( (seed == 0) ? random_device() : seed );
        // for now, only choose from directions 3, 4, and 5 which are the directions defined to only
        // move in the positive directions (see Constellation::Adjacency)
        std::uniform_int_distribution rng(3, 5); 
        
        // the bytes we will be saving to obervatory_adjacency_shader_texture
        godot::PackedByteArray adjacencies_texture_data;
        adjacencies_texture_data.resize(tracks_size * tracks_size);
        adjacencies_texture_data.fill(0);
        uint8_t* ptr = adjacencies_texture_data.ptrw();

        godot::Vector2i current_id { 0, 0 };
        for(int i = 0; i < tracks_size; i++)
        {
            godot::Ref<Track> track = tracks[i];

            covers.emplace_back(track->get_album()->get_cover());
            ids.emplace_back(current_id);
            
            // each Track goes to one other, in a random forward direction
            int random_number = rng(device);

            if( i < tracks_size-1 ) // if not last track
            {
                adjacencies.emplace_back( 1 << random_number ); // set the direction to the bit the rng chose
                ptr[ current_id.x*tracks_size + current_id.y ] = adjacencies[i]; // and save that direction at current position

                // finally, move the next track from the random direction chosen
                if     (random_number == 3) { current_id.x += 1; }
                else if(random_number == 4) { current_id.x += 1; current_id.y += 1; }
                else if(random_number == 5) {                    current_id.y += 1; }
                //else if(random_number == 6) { current_id.x -= 1; current_id.y += 1; }
                //else if(random_number == 7) { current_id.x -= 1; }
            }
            else adjacencies.emplace_back( 0 );
        }
        
        // save adjacencies_texture_data as an image
        return; // jk skip it
        godot::Ref<godot::Image> texture_image = godot::Image::create_from_data(tracks_size, tracks_size, false, godot::Image::FORMAT_R8, adjacencies_texture_data);
        // save image to adjacency texture, this is what finally is passed to the adjacency shader
        if(observatory_adjacency_shader_texture.is_valid()) observatory_adjacency_shader_texture->set_image(texture_image);
        else observatory_adjacency_shader_texture = godot::ImageTexture::create_from_image(texture_image);
        //texture_image->save_png("res://adjacencies.png"); // save to disk for debug
    }
    
    bool is_initialized() const { return !ids.empty() && !covers.empty() && ids.size() == tracks.size() && covers.size() == tracks.size(); }

    godot::TypedArray<Track> get_tracks() const { return tracks; }
    void set_tracks(const godot::TypedArray<Track>& p_tracks) { tracks = p_tracks; }
    
    int get_seed() const { return seed; }
    void set_seed(int p_seed) { seed = p_seed; cache(); }
}; // Constellation

} // rhythm

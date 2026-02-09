#pragma once

#include <vector>
#include <string>

namespace rhythm
{

struct Model
{
    static constexpr int NULL_ID = -1;
    int id = NULL_ID;
}; //


struct Track : public Model
{
    /* member variables */
    std::string title;
    std::string fingerprint;
    
    /* foreign key */
    int album_id = Model::NULL_ID;
}; // Track

struct Album : public Model
{
    /* member variables */
    enum Type : uint8_t { SINGLE = 0, EP = 1, LP = 2 };
    Type type;
    std::string title;
    int release_year = 0, release_month = 0, release_day = 0;

    /* child objects */
    std::vector<Track> tracklist;
    
    /* foreign key */
    int artist_id = Model::NULL_ID;
}; // Album

struct Artist : public Model
{
    /* member variables */
    std::string name;
    
    /* child objects */
    std::vector<Album> albums;
}; // Artist

} // rhythm
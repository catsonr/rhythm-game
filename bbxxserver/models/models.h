#pragma once

#include <vector>
#include <string>

namespace server
{

static constexpr int NULL_ID = -1;

struct Track
{
    /* primary key */
    int id = NULL_ID;

    /* member variables */
    std::string title;
    std::string fingerprint;
    
    /* foreign key */
    int album_id = NULL_ID;
}; // Track

struct Album
{
    /* primary key */
    int id = NULL_ID;
    
    /* member variables */
    enum Type : uint8_t { SINGLE = 0, EP = 1, LP = 2 };
    Type type;
    std::string title;
    int release_year = 0, release_month = 0, release_day = 0;
    
    /* foreign key */
    int artist_id = NULL_ID;

    /* child objects */
    std::vector<Track> tracklist;
}; // Album

struct Artist
{
    /* primary key */
    int id = NULL_ID;
    
    /* member variables */
    std::string name;
    
    /* child objects */
    std::vector<Album> albums;
}; // Artist

} // server
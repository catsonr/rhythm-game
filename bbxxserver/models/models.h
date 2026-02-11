#pragma once

#include <vector>
#include <string>
#include <optional>

namespace server
{

static constexpr uint64_t NULL_ID = 0;

struct Track
{
    /* primary key */
    uint64_t id = NULL_ID;

    /* member variables */
    std::string title;
    std::string fingerprint;
    
    /* foreign key */
    uint64_t ALBUM_ID = NULL_ID;
}; // Track

struct Album
{
    /* primary key */
    uint64_t id = NULL_ID;
    
    /* member variables */
    enum Type : uint8_t { SINGLE = 0, EP = 1, LP = 2 };
    uint type; // type_orm prefers int -- typecast to Album::Type for comparisons
    std::string title;
    std::optional<int> release_year = 0, release_month = 0, release_day = 0;
    std::optional<std::string> MBID; // https://musicbrainz.org/doc/MusicBrainz_Identifier
    
    /* foreign key */
    uint64_t ARTIST_ID = NULL_ID;

    /* child objects */
    std::vector<Track> tracklist;
}; // Album

struct Artist
{
    /* primary key */
    uint64_t id = NULL_ID;
    
    /* member variables */
    std::string name; // UNIQUE!!!
    
    /* child objects */
    std::vector<Album> albums;
}; // Artist

} // server
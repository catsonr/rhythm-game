#pragma once

/*
    bbxxserver/models/models.h is the single source of truth for what a beatboxx resource, be it a
    Artist, Chart, User, etc., can possibly be
    
    from these structs, they can be rehydrated into actual runtime objects. see:
    bbxxserver/models/storage.h for hydration into sqlite_orm, and
    src/ for godot specific resources (e.g., Track.h)
*/

#include <vector>
#include <string>
#include <optional>
#include <filesystem>

namespace server
{

static constexpr uint64_t NULL_ID = 0;

struct Artist
{
    /* primary key */
    uint64_t id = NULL_ID;
    
    /* member variables */
    std::string name; // UNIQUE!!!
    // std::optional<Area> area; // see musicbrainz Area
    
    /* HAS CHILDREN Album */
}; // Artist

struct Album
{
    /* primary key */
    uint64_t id = NULL_ID;
    
    /* member variables */
    enum Type : uint8_t { SINGLE = 0, EP = 1, LP = 2 };
    uint8_t type; // type_orm prefers int -- typecast to Album::Type for comparisons
    std::string title;
    std::optional<int> release_year = 0, release_month = 0, release_day = 0;
    std::optional<std::string> releasegroup_MBID; // https://musicbrainz.org/doc/MusicBrainz_Identifier
    std::optional<std::filesystem::path> cover_path;
    
    /* foreign key */
    uint64_t ARTIST_ID = NULL_ID;
    
    /* HAS CHILDREN Track */
}; // Album

struct Track
{
    /* primary key */
    uint64_t id = NULL_ID;

    /* member variables */
    std::string title;
    
    /* foreign key */
    uint64_t ALBUM_ID = NULL_ID;
    
    /* HAS CHILDREN Chart */
}; // Track

struct Chart
{
    /* primary key */
    uint64_t id = NULL_ID;
    
    /* member variables */
    // see Track.h
    std::vector<uint64_t> beats;
    std::vector<uint64_t> notes;
    
    /* foreign key */
    uint64_t TRACK_ID = NULL_ID;
    uint64_t USER_ID = NULL_ID;
}; // Chart

struct User
{
    /* primary key*/
    uint64_t id = NULL_ID;
    
    /* member variables */
    std::string username; // UNIQUE!!
    std::optional<std::string> email;
    std::string password_hash;
    // UserSettings user_settings;
    
    /* HAS CHILDREN Chart */
}; // User

struct UserSession
{
    /* primary key */
    std::string uuid;
    
    /* member variables */
    int64_t expire_time = 0; // unix time of expiration, in seconds since epoch
    
    /* foreign key */
    uint64_t USER_ID = NULL_ID;
    
    /* lemmas */
    bool expired(const int64_t seconds_since_epoch) const { return seconds_since_epoch >= expire_time; }
}; // UserSession

} // server
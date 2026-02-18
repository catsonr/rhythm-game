#include <drogon/drogon.h>

#include "models/storage.h"

int main()
{
    // populate db with some entires
    server::Storage storage = server::init_storage();
    storage.sync_schema();
    if( storage.count<server::Artist>() == 0 )
    {
        // artists
        server::Artist artist;
        artist.name = "Radiohead";
        storage.insert(artist); // artist 1
        artist.name = "Aphex Twin";
        storage.insert(artist); // artist 2
        
        // albums
        server::Album album;
        album.title = "The Bends";
        album.type = server::Album::Type::LP;
        album.ARTIST_ID = 1;
        storage.insert(album); // album 1
        album.title = "Selected Ambient Works 85-92";
        album.type = server::Album::Type::LP;
        album.ARTIST_ID = 2;
        storage.insert(album); // album 2
        
        // tracks
        server::Track track;
        track.title = "Just";
        track.ALBUM_ID = 1;
        storage.insert(track); // track 1
        track.title = "My Iron Lung";
        track.ALBUM_ID = 1;
        storage.insert(track); // track 2
        track.title = "Xtal";
        track.ALBUM_ID = 2;
        storage.insert(track); // track 3
        track.title = "Pulsewidth";
        track.ALBUM_ID = 2;
        storage.insert(track); // track 4
    }

    // increase max body size for big audio file uploads
    drogon::app().setClientMaxBodySize(20 * 1024*1024); // 20mb

    // start server
    LOG_INFO << "server running on 0.0.0.0:3939";
    drogon::app().addListener("0.0.0.0", 3939).run();
}
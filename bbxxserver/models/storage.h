#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "../sqlite_orm.h"
#pragma clang diagnostic pop

#include "models.h"

namespace server
{

// determines how std::optional is serialized into json
template <typename T>
Json::Value value_or_null(const std::optional<T>& t) { return t.has_value() ? Json::Value(*t) : Json::Value::null; }

inline auto init_storage()
{
    using namespace sqlite_orm;
    return make_storage("bx.db",
        make_table("artists",
            /* primary key */
            make_column("id", &Artist::id, primary_key().autoincrement()),

            /* member variables */
            make_column("name", &Artist::name, unique())
        ),

        make_table("albums",
            /* primary key*/
            make_column("id", &Album::id, primary_key().autoincrement()),

            /* member variables */
            make_column("type", &Album::type),
            make_column("title", &Album::title),
            make_column("release_year", &Album::release_year),
            make_column("release_month", &Album::release_month),
            make_column("release_day", &Album::release_day),
            // cover path

            /* foreign key */
            make_column("artist_id", &Album::ARTIST_ID),
            foreign_key(&Album::ARTIST_ID).references(&Artist::id)
        ),

        make_table("tracks",
            /* primary key */
            make_column("id", &Track::id, primary_key().autoincrement()),

            /* member variables */
            make_column("title", &Track::title),
            make_column("fingerprint", &Track::fingerprint),

            /* foreign key */
            make_column("album_id", &Track::ALBUM_ID),
            foreign_key(&Track::ALBUM_ID).references(&Album::id)
        )

        // charts
        // users
    );
}

using Storage = decltype( init_storage() );

} // server
#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "../sqlite_orm.h"
#pragma clang diagnostic pop

#include "models.h"

namespace sqlite_orm
{
    template<>
    struct type_printer<server::Album::Type>
    {
        const std::string& print()
        {
            static const std::string res = "INTEGER";
            return res;
        }
    };
}

namespace server
{

inline auto init_storage()
{
    using namespace sqlite_orm;
    return make_storage("bx.db",
        make_table("artists",
            /* primary key */
            make_column("id", &Artist::id, primary_key().autoincrement()),

            /* member variables */
            make_column("name", &Artist::name)
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

            /* foreign key */
            make_column("artist_id", &Album::artist_id),
            foreign_key(&Album::artist_id).references(&Artist::id)
        ),

        make_table("tracks",
            /* primary key */
            make_column("id", &Track::id, primary_key().autoincrement()),

            /* member variables */
            make_column("title", &Track::title),
            make_column("fingerprint", &Track::fingerprint),

            /* foreign key */
            make_column("album_id", &Track::album_id),
            foreign_key(&Track::album_id).references(&Album::id)
        )
    );
}

} // server
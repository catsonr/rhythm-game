#pragma once

#include "../sqlite_orm.h"

#include "models.h"

namespace rhythm
{

inline auto init_storage()
{
    using namespace sqlite_orm;
    return make_storage("bx.db",
        make_table("tracks",
            make_column("id", &Track::id, primary_key().autoincrement()),
            make_column("title", &Track::title),
            make_column("fingerprint", &Track::fingerprint)
        )
    );
}

} // rhythm
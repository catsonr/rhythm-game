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
static Json::Value value_or_null(const std::optional<T>& t) { return t.has_value() ? Json::Value(*t) : Json::Value::null; }

static std::string vector_uint64_to_base64(const std::vector<uint64_t>& v)
{
    if( v.empty() ) return "";
    
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(v.data());
    size_t size = v.size() * sizeof(uint64_t);
    
    return drogon::utils::base64Encode(bytes, size);
}
static std::vector<uint64_t> base64_to_vector_uint64(const std::string& s)
{
    if( s.empty() ) return {};
    
    
    std::vector<char> bytes = drogon::utils::base64DecodeToVector(s);
    if(bytes.size() % sizeof(uint64_t) != 0)
    {
        LOG_ERROR << "[server::base64_to_vector_uint64] too many/too few bytes! ignoring ...";
        return {};
    }
    
    size_t size = bytes.size() / sizeof(uint64_t);
    std::vector<uint64_t> result(size);
    std::memcpy(result.data(), bytes.data(), bytes.size());
    
    return result;
}

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

            /* foreign key */
            make_column("album_id", &Track::ALBUM_ID),
            foreign_key(&Track::ALBUM_ID).references(&Album::id)
        ),

        make_table("charts",
            /* primary key */
            make_column("id", &Chart::id, primary_key().autoincrement()),

            /* member variables */
            make_column("beats", &Chart::beats),
            make_column("notes", &Chart::notes),

            /* foreign key */
            make_column("track_id", &Chart::TRACK_ID),
            make_column("user_id", &Chart::USER_ID),

            foreign_key(&Chart::TRACK_ID).references(&Track::id),
            foreign_key(&Chart::USER_ID).references(&User::id)
        ),
        
        make_table("users",
            /* primary key */
            make_column("id", &User::id, primary_key().autoincrement()),

            /* member variables */
            make_column("username", &User::username),
            make_column("email", &User::email),
            make_column("password_hash", &User::password_hash)
        ),

        make_table("user_sessions",
            /* primary key */
            make_column("uuid", &UserSession::uuid, primary_key()),

            /* member variables */
            make_column("expire_time", &UserSession::expire_time),

            /* foreign key */
            make_column("USER_ID", &UserSession::USER_ID),
            foreign_key(&UserSession::USER_ID).references(&User::id)
        )
    );
}

using Storage = decltype( init_storage() );

} // server

namespace sqlite_orm
{
    /* tells sqlite_orm how to handle std::vector<uint64_t> */

    template<>
    struct type_printer<std::vector<uint64_t>>
    {
        const std::string& print() const
        {
            static const std::string type { "BLOB" };
            return type;
        }
    }; // type_printer

    template<>
    struct statement_binder<std::vector<uint64_t>>
    {
        int bind(sqlite3_stmt* stmt, int index, const std::vector<uint64_t>& value) const
        {
            return sqlite3_bind_blob(
                stmt,
                index,
                value.data(),
                value.size() * sizeof(uint64_t),
                SQLITE_TRANSIENT
            );
        }
    }; // statement_binder
    
    template<>
    struct row_extractor<std::vector<uint64_t>>
    {
        std::vector<uint64_t> extract(sqlite3_stmt* stmt, int index) const
        {
            const void* column_ptr = sqlite3_column_blob(stmt, index);
            int column_bytes = sqlite3_column_bytes(stmt, index);
            
            if( !column_ptr )
            {
                LOG_ERROR << "[sqlite_orm::row_extractor<uint64_t>::extract] could not find column at index " + std::to_string(index) + "! ignoring ..."; 
                return {};
            }
            if( column_bytes == 0 )
            {
                printf("[sqlite_orm::row_extractor<uint64_t>::extract] attempted to extract 0 bytes at index %i! ignoring ...", index);
                return {};
            }

            size_t size = column_bytes / sizeof(uint64_t);
            std::vector<uint64_t> result(size);
            std::memcpy(result.data(), column_ptr, column_bytes);
            
            return result;
        }
    }; // row_extractor
    
    template<>
    struct field_printer<std::vector<uint64_t>>
    {
        std::string operator()(const std::vector<uint64_t>& buffer) const
        {
            return "compiler wants this but im just going to hope it's unneeded. if you're reading this, it's needed!";
        }
    }; // field_printer
}
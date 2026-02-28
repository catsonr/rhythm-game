#pragma once

#include <godot_cpp/variant/vector4.hpp>

namespace rhythm
{

// forward declarations
struct AudioEngine2;
struct BXApi;

struct BXCTX
{
    AudioEngine2* audio_engine_2 { nullptr };
    BXApi* bxapi { nullptr };

    godot::Vector4 G { 3, 0, 0, 3 };

    static BXCTX& get()
    {
        static BXCTX ctx; // this is THE global context!

        return ctx;
    }
}; // BXCTX

} // rhythm
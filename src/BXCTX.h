#pragma once

#include <godot_cpp/variant/vector4.hpp>

#include "BXApi.h"
#include "AudioEngine2.h"

namespace rhythm
{

struct SceneManager; // forward declare for BXCTX

struct BXCTX
{
    SceneManager* scene_manager { nullptr };
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
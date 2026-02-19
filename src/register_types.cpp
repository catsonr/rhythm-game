#include "register_types.h"

// nodes
#include "AudioEngine2.h"
#include "AudioEngine2_Pause_Shader.h"
#include "BeatEditor.h"
#include "BXApi.h"
#include "LoginWindow.h"
#include "NoteEditor.h"
#include "Observatory.h"
#include "SceneManager.h"
#include "Taiko2.h"
#include "TitleScreen.h"

// resources
#include "Album.h"
#include "Audio.h"
#include "Track.h"
#include "UserSession.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_example_module(ModuleInitializationLevel p_level)
{
    if(p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    
    // register nodes
    GDREGISTER_RUNTIME_CLASS(rhythm::AudioEngine2);
    GDREGISTER_RUNTIME_CLASS(rhythm::AudioEngine2_Pause_Shader);
    GDREGISTER_RUNTIME_CLASS(rhythm::BeatEditor);
    GDREGISTER_RUNTIME_CLASS(rhythm::BXApi);
    GDREGISTER_RUNTIME_CLASS(rhythm::LoginWindow);
    GDREGISTER_RUNTIME_CLASS(rhythm::NoteEditor);
    GDREGISTER_RUNTIME_CLASS(rhythm::Observatory);
    GDREGISTER_RUNTIME_CLASS(rhythm::Scene);
    GDREGISTER_RUNTIME_CLASS(rhythm::SceneManager);
    GDREGISTER_RUNTIME_CLASS(rhythm::Taiko2);
    GDREGISTER_RUNTIME_CLASS(rhythm::TitleScreen);
    
    // register resources
    GDREGISTER_CLASS(rhythm::Album);
    GDREGISTER_CLASS(rhythm::Audio);
    GDREGISTER_CLASS(rhythm::Constellation);
    GDREGISTER_CLASS(rhythm::Track);
    GDREGISTER_CLASS(rhythm::UserSession);
}

void uninitialize_example_module(ModuleInitializationLevel p_level)
{
    if(p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
}

extern "C"
{
    GDExtensionBool GDE_EXPORT example_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
        
        init_obj.register_initializer(initialize_example_module);
        init_obj.register_terminator(uninitialize_example_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
        
        return init_obj.init();
    }
}
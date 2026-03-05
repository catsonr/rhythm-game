#include "register_types.h"

/* godot::Resource !*/
#include "Album.h"
#include "Audio.h"
#include "Constellation.h"
#include "Track.h"
#include "UserSession.h"

/* godot::Node !*/
#include "AudioEngine2.h"
#include "AudioEngine2_Pause_Shader.h"
#include "BeatEditor.h"
#include "BXApi.h"
#include "LoginWindow.h"
#include "NoteEditor.h"
#include "Taiko2.h"

/* DSP ( digital signal processing! ) */
#include "nodes/dsp/DSPGraphEdit.h"
#include "nodes/dsp/DSPGraphEditor.h"
#include "nodes/dsp/Multiplier.h"
#include "nodes/dsp/Oscillator.h"
#include "nodes/dsp/Output.h"

/* SM ( scene machine! ) */
#include "nodes/sm/BXScene.h"
#include "nodes/sm/Observatory.h"
#include "nodes/sm/SceneMachine.h"
#include "nodes/sm/TitleScreen.h"

/* godot !*/
#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void inline register_nodes()
{
    /* DSP */
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::DSPGraphEdit);
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::DSPGraphEditor);
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::DSPGraphNode);
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::MultiplierGraphNode);
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::OscillatorGraphNode);
    GDREGISTER_RUNTIME_CLASS(rhythm::dsp::OutputGraphNode);
    
    /* SM */
    GDREGISTER_RUNTIME_CLASS(rhythm::sm::BXScene);
    GDREGISTER_RUNTIME_CLASS(rhythm::sm::Observatory);
    GDREGISTER_RUNTIME_CLASS(rhythm::sm::SceneMachine);
    GDREGISTER_RUNTIME_CLASS(rhythm::sm::TitleScreen);
    
    /* godot::Node */
    GDREGISTER_RUNTIME_CLASS(rhythm::AudioEngine2);
    GDREGISTER_RUNTIME_CLASS(rhythm::AudioEngine2_Pause_Shader);
    GDREGISTER_RUNTIME_CLASS(rhythm::BeatEditor);
    GDREGISTER_RUNTIME_CLASS(rhythm::BXApi);
    GDREGISTER_RUNTIME_CLASS(rhythm::LoginWindow);
    GDREGISTER_RUNTIME_CLASS(rhythm::NoteEditor);
    GDREGISTER_RUNTIME_CLASS(rhythm::Taiko2);
}

void inline register_resources()
{
    GDREGISTER_CLASS(rhythm::Album);
    GDREGISTER_CLASS(rhythm::Audio);
    GDREGISTER_CLASS(rhythm::Constellation);
    GDREGISTER_CLASS(rhythm::Track);
    GDREGISTER_CLASS(rhythm::UserSession);
}

void initialize_beatboxx_module(ModuleInitializationLevel p_level)
{
    if(p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    
    register_resources();
    register_nodes();
}

void uninitialize_beatboxx_module(ModuleInitializationLevel p_level)
{
    if(p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
}

extern "C"
{
    GDExtensionBool GDE_EXPORT beatboxx_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
        
        init_obj.register_initializer(initialize_beatboxx_module);
        init_obj.register_terminator(uninitialize_beatboxx_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
        
        return init_obj.init();
    }
}
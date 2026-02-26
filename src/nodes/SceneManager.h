#pragma once

#include <stack>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "BXCTX.h"
#include "BXApi.h"
#include "AudioEngine2.h"

namespace rhythm
{

struct SceneManager : public godot::Node
{
    GDCLASS(SceneManager, godot::Node)

private:
    godot::NodePath audio_engine_2_path;
    godot::NodePath bxapi_path;

    godot::Ref<godot::PackedScene> initial_scene;
    
    std::stack<godot::Node*> scenes;

public:
    void _ready() override
    {
        BXCTX::get().scene_manager = this;

        // audio engine 2 stuff
        if( audio_engine_2_path.is_empty() ) { godot::print_error("[SceneManager::_ready] a NodePath to AudioEngine2 has not been set. please set one in the inspector!"); return; }
        godot::Node* audio_engine_2_node = get_node_or_null(audio_engine_2_path);
        AudioEngine2* audio_engine_2 = godot::Object::cast_to<rhythm::AudioEngine2>(audio_engine_2_node);
        if( !audio_engine_2 ) { godot::print_error("[SceneManager::_ready] a path to AudioEngine2 has been set, but is it not a rhythm::AudioEngine2!"); return; }
        
        BXCTX::get().audio_engine_2 = audio_engine_2;
        
        // bxapi stuff
        if( bxapi_path.is_empty() ) { godot::print_error("[SceneManager::_ready] a NodePath to BXApi has not been set. please set one in the inspector!"); return; }
        godot::Node* bxapi_node = get_node_or_null(bxapi_path);
        BXApi* bxapi = godot::Object::cast_to<rhythm::BXApi>(bxapi_node);
        if( !bxapi ) { godot::print_error("[SceneManager::_ready] a path to BXApi has been set, but is it not a rhythm::BXApi!"); return; }
        
        BXCTX::get().bxapi = bxapi;
        
        // initial scene stuff
        if(initial_scene.is_valid()) push_scene(initial_scene, true);
        else godot::print_line("[SceneManager::_ready] no initial scene is set... nothing to do!");
    }
    
    // if keep_processing is true, the scene before p_scene will continue to be processed
    void push_scene(godot::Ref<godot::PackedScene> p_scene, bool keep_processing)
    {
        if(p_scene.is_null()) { godot::print_error("[SceneManager::change_scene] p_scene is null!"); return; }
        
        // instantiate the scene 
        godot::Node* scene = p_scene->instantiate();
        
        // disable the current scene (if neccessary)
        if( !scenes.empty() && !keep_processing ) disable_scene( scenes.top() );
        
        // finally, pass scene to godot (calls _ready() )
        add_child(scene);
        // and push to stack
        scenes.push(scene);
        
        // TODO: have this print the name of the scene, somehow
        godot::print_line("[SceneManager::push_scene] pushed new scene!");
    }
    
    void pop_scene()
    {
        if( !scenes.empty() )
        {
            godot::Node* current_scene = scenes.top();
            godot::String current_scene_name = current_scene->get_name();
            
            scenes.pop();
            remove_child(current_scene);
            current_scene->queue_free();

            godot::print_line("[SceneManager::pop_scene] popped scene '" + current_scene_name + "'!");
            
            if( !scenes.empty() ) enable_scene( scenes.top() );
        }
        else godot::print_line("[SceneManager::pop_scene] no scene to pop!");
    }
    
    static void enable_scene(godot::Node* scene)
    {
        scene->set_process_mode(Node::PROCESS_MODE_INHERIT);
        //show();

        godot::print_line("[SceneManager::enable_scene] enabled a scene!");
    }
    
    static void disable_scene(godot::Node* scene)
    {
        scene->set_process_mode(Node::PROCESS_MODE_DISABLED);
        //hide();

        godot::print_line("[SceneManager::disable_scene] disabled a scene!");
    }

    godot::NodePath get_audio_engine_2_path() const { return audio_engine_2_path; }
    void set_audio_engine_2_path(const godot::NodePath& p_audio_engine_2_path) { audio_engine_2_path = p_audio_engine_2_path; }

    godot::NodePath get_bxapi_path() const { return bxapi_path; }
    void set_bxapi_path(const godot::NodePath& p_bxapi_path) { bxapi_path = p_bxapi_path; }

    godot::Ref<godot::PackedScene> get_initial_scene() const { return initial_scene; }
    void set_initial_scene(const godot::Ref<godot::PackedScene>& p_initial_scene) { initial_scene = p_initial_scene; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_2_path"), &rhythm::SceneManager::get_audio_engine_2_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_2_path", "p_audio_engine_2_path"), &rhythm::SceneManager::set_audio_engine_2_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_2_path"), "set_audio_engine_2_path", "get_audio_engine_2_path");

        godot::ClassDB::bind_method(godot::D_METHOD("get_bxapi_path"), &rhythm::SceneManager::get_bxapi_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_bxapi_path", "p_bxapi_path"), &rhythm::SceneManager::set_bxapi_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "bxapi_path"), "set_bxapi_path", "get_bxapi_path");

        godot::ClassDB::bind_method(D_METHOD("get_initial_scene"), &rhythm::SceneManager::get_initial_scene);
        godot::ClassDB::bind_method(D_METHOD("set_initial_scene", "p_initial_scene"), &rhythm::SceneManager::set_initial_scene);
        ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "initial_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_initial_scene", "get_initial_scene");
    }
}; // SceneManager

} // rhythm

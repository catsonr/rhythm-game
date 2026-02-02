#pragma once

#include <stack>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "AudioEngine2.h"

namespace rhythm
{

struct SceneManager; // forward declare for CTX

/*
    the global rhythm context
    TODO: enforce existance of CTX member variables
*/
struct CTX
{
    SceneManager* scene_manager;
    AudioEngine2* audio_engine_2 { nullptr };
    godot::Vector4 G { 3, 0, 0, 3 };
}; // CTX

/*
    Scene is to be the root node of any godot Scene
    each Scene has a pointer to CTX, this means that we can be sure that any rhythm::Scene has a
    valid reference to any global variable needed!
    thus, any godot scene whose root node is rhythm::Scene, need not worry about where their
    AudioEngine, or lattice matrix G is coming from. they can simply use it
*/
struct Scene : public godot::Control
{
    GDCLASS(Scene, godot::Control)

private:
    CTX* ctx { nullptr };

protected:
    static void _bind_methods() {}

public:
    static CTX* conjure_ctx(const godot::Node* node)
    {
        if(!node) { godot::print_error("[Scene::conjure_ctx] cannot get the ctx of a null node!"); return nullptr; }
        
        godot::Node* owner = node->get_owner();
        rhythm::Scene* scene = godot::Object::cast_to<rhythm::Scene>(owner);
        
        if(scene) return scene->get_ctx();
        
        godot::print_error("[Scene::conjure_ctx] failed to get ctx, the provided node is not owned by a rhythm::Scene! (is the root node of your godot scene of type rhythm::Scene?)");
        
        return nullptr;
    }
    
    void enable()
    {
        set_process_mode(Node::PROCESS_MODE_INHERIT);
        //show();

        godot::print_line("[Scene::enable] enabled a scene!");
    }
    
    void disable()
    {
        set_process_mode(Node::PROCESS_MODE_DISABLED);
        //hide();

        godot::print_line("[Scene::disable] disabled a scene!");
    }

    CTX* get_ctx() const { return ctx; }
    void set_ctx(CTX* p_ctx) { ctx = p_ctx; }
}; // Scene

/*
    SceneManager will handle loading and swapping any godot scenes, provided their root node is a rhythm::Scene
*/
struct SceneManager : public godot::Node
{
    GDCLASS(SceneManager, godot::Node)

private:
    godot::NodePath audio_engine_path;
    godot::NodePath audio_engine_2_path;
    CTX ctx; // this is THE global context!

    godot::Ref<godot::PackedScene> initial_scene;
    
    std::stack<Scene*> scenes;

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_path"), &rhythm::SceneManager::get_audio_engine_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_path", "p_audio_engine_path"), &rhythm::SceneManager::set_audio_engine_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_path"), "set_audio_engine_path", "get_audio_engine_path");

        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_2_path"), &rhythm::SceneManager::get_audio_engine_2_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_2_path", "p_audio_engine_2_path"), &rhythm::SceneManager::set_audio_engine_2_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_2_path"), "set_audio_engine_2_path", "get_audio_engine_2_path");

        godot::ClassDB::bind_method(godot::D_METHOD("get_G"), &rhythm::SceneManager::get_G);
        godot::ClassDB::bind_method(godot::D_METHOD("set_G", "p_G"), &rhythm::SceneManager::set_G);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR4, "G"), "set_G", "get_G");
        
        godot::ClassDB::bind_method(D_METHOD("get_initial_scene"), &rhythm::SceneManager::get_initial_scene);
        godot::ClassDB::bind_method(D_METHOD("set_initial_scene", "p_initial_scene"), &rhythm::SceneManager::set_initial_scene);
        ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "initial_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_initial_scene", "get_initial_scene");
    }

public:
    void _ready() override
    {
        // audio engine 2 stuff

        if( audio_engine_2_path.is_empty() ) { godot::print_error("[SceneManager::_ready] a NodePath to AudioEngine2 has not been set. please set one in the inspector!"); return; }
        
        godot::Node* node_2 = get_node_or_null(audio_engine_2_path);
        AudioEngine2* audio_engine_2 = godot::Object::cast_to<rhythm::AudioEngine2>(node_2);
        
        if( !audio_engine_2 ) { godot::print_error("[SceneManager::_ready] a path to AudioEngine2 has been set, but is it not an AudioEngine2!"); return; }
        
        ctx.audio_engine_2 = audio_engine_2;
        
        // initial scene stuff

        if(initial_scene.is_valid()) push_scene(initial_scene, true);
        else godot::print_line("[SceneManager::_ready] no initial scene is set... nothing to do!");
        
        // scene_manager pointer stuff
        ctx.scene_manager = this;
    }
    
    // if keep_processing is true, the scene before p_scene will continue to be processed
    void push_scene(godot::Ref<godot::PackedScene> p_scene, bool keep_processing)
    {
        if(p_scene.is_null()) { godot::print_error("[SceneManager::change_scene] p_scene is null!"); return; }
        
        // instantiate the scene 
        godot::Node* node = p_scene->instantiate();

        // check if this scene is a rhythm::Scene
        rhythm::Scene* scene = godot::Object::cast_to<rhythm::Scene>(node);
        if(!scene)
        {
            godot::print_error("[SceneManager::chance_scene] failed to change scene. new scene is not of type rhythm::Scene! (is the ROOT NODE of your scene a rhythm::Scene?)");
            
            node->queue_free();
            return;
        }
        
        // disable the current scene (if neccessary)
        if( !scenes.empty() && !keep_processing ) scenes.top()->disable();
        
        // give new scene a pointer to the global context
        scene->set_ctx(&ctx);
        
        // finally, pass scene to godot (calls _ready() )
        add_child(scene);
        // and push to stack
        scenes.push(scene);
        
        godot::print_line("[SceneManager::push_scene] pushed new scene '" + scene->get_name() + "'!");
    }
    
    void pop_scene()
    {
        if( !scenes.empty() )
        {
            Scene* current_scene = scenes.top();
            godot::String current_scene_name = current_scene->get_name();
            
            scenes.pop();
            remove_child(current_scene);
            current_scene->queue_free();

            godot::print_line("[SceneManager::pop_scene] popped scene '" + current_scene_name + "'!");
            
            if( !scenes.empty() ) scenes.top()->enable();
        }
        else godot::print_line("[SceneManager::pop_scene] no scene to pop!");
    }

    godot::NodePath get_audio_engine_path() const { return audio_engine_path; }
    void set_audio_engine_path(const godot::NodePath& p_audio_engine_path) { audio_engine_path = p_audio_engine_path; }

    godot::NodePath get_audio_engine_2_path() const { return audio_engine_2_path; }
    void set_audio_engine_2_path(const godot::NodePath& p_audio_engine_2_path) { audio_engine_2_path = p_audio_engine_2_path; }
    
    godot::Vector4 get_G() const { return ctx.G; }
    void set_G(const godot::Vector4& p_G) { ctx.G = p_G; }

    godot::Ref<godot::PackedScene> get_initial_scene() const { return initial_scene; }
    void set_initial_scene(const godot::Ref<godot::PackedScene>& p_initial_scene) { initial_scene = p_initial_scene; }
}; // SceneManager

} // rhythm

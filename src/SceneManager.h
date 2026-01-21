#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "AudioEngine.h"

namespace rhythm
{

/*
    the global rhythm context
*/
struct CTX
{
    AudioEngine* audio_engine { nullptr };
    godot::Vector4 G { 1, 0, 0, 1 };
}; // CTX

/*
    Scene is to be the root node of any godot Scene
    each Scene has a pointer to CTX, this means that we can be sure that any rhythm::Scene has a
    valid reference to any global variable needed!
    thus, any godot scene whose root node is rhythm::Scene, need not worry about where their
    AudioEngine, or lattice matrix G is coming from. they can simply use it
*/
struct Scene : public godot::Node
{
    GDCLASS(Scene, godot::Node)

private:
    CTX* ctx { nullptr };

protected:
    static void _bind_methods() {}

public:
    static CTX* conjure_ctx(godot::Node* node)
    {
        if(!node) { godot::print_error("[Scene::conjure_ctx] cannot get the ctx of a null node!"); return nullptr; }
        
        godot::Node* owner = node->get_owner();
        rhythm::Scene* scene = godot::Object::cast_to<rhythm::Scene>(owner);
        
        if(scene) return scene->get_ctx();
        
        godot::print_error("[Scene::conjure_ctx] failed to get ctx, the provided node is not owned by a rhythm::Scene! (is the root node of your godot scene of type rhythm::Scene?)");
        
        return nullptr;
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
    CTX ctx; // this is THE global context!

    Scene* current_scene { nullptr };
    godot::Ref<godot::PackedScene> initial_scene;

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_audio_engine_path"), &rhythm::SceneManager::get_audio_engine_path);
        godot::ClassDB::bind_method(godot::D_METHOD("set_audio_engine_path", "p_audio_engine_path"), &rhythm::SceneManager::set_audio_engine_path);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::NODE_PATH, "audio_engine_path"), "set_audio_engine_path", "get_audio_engine_path");
        
        godot::ClassDB::bind_method(D_METHOD("get_initial_scene"), &rhythm::SceneManager::get_initial_scene);
        godot::ClassDB::bind_method(D_METHOD("set_initial_scene", "p_initial_scene"), &rhythm::SceneManager::set_initial_scene);
        ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "initial_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_initial_scene", "get_initial_scene");
    }

public:
    void _ready() override
    {
        // audio engine stuff

        if( audio_engine_path.is_empty() ) { godot::print_error("[SceneManager::_ready] a NodePath to AudioEngine has not been set. please set one in the inspector!"); return; }
        
        godot::Node* node = get_node_or_null(audio_engine_path);
        AudioEngine* audio_engine = godot::Object::cast_to<rhythm::AudioEngine>(node);
        
        if( !audio_engine ) { godot::print_error("[SceneManager::_ready] a path to AudioEngine has been set, but is it not an AudioEngine!"); return; }
        
        ctx.audio_engine = audio_engine;
        
        // initial scene stuff

        if(initial_scene.is_valid()) change_scene(initial_scene);
        else godot::print_line("[SceneManager::_ready] no initial scene is set... nothing to do!");
    }
    
    void change_scene(godot::Ref<godot::PackedScene> p_scene)
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
        
        // free any previous scene
        if(current_scene) { current_scene->queue_free(); current_scene = nullptr; }
        
        // give new scene a pointer to the global context
        scene->set_ctx(&ctx);
        
        current_scene = scene;
        
        // finally, pass scene to godot (calls _ready() )
        add_child(scene);
    }

    godot::NodePath get_audio_engine_path() const { return audio_engine_path; }
    void set_audio_engine_path(const godot::NodePath& p_audio_engine_path) { audio_engine_path = p_audio_engine_path; }

    godot::Ref<godot::PackedScene> get_initial_scene() const { return initial_scene; }
    void set_initial_scene(const godot::Ref<godot::PackedScene>& p_initial_scene) { initial_scene = p_initial_scene; }
}; // SceneManager

} // rhythm

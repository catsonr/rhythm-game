#pragma once

/*
   SceneMachine is a finite state machine where each possible state is a rhythm::sm::BXScene, and
   each BXScene is itself just a godot::Control
   
   state is controlled with enter_scene() and exit_scene()
   
   SceneMachine, on top of its BXScene current_scene, has a stack<BXScene> stack, making this a pushdown automata.
   all that to say:
   SceneMachine can assume a single state. on top of that state, you may push and pop additional states as you wish
   this is useful for options menus, HUDs, etc
   
   stack is controlled with push_scene() and pop_scene()
*/

#include <stack>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "BXScene.h"

namespace rhythm::sm
{

struct Transition
{
    godot::Ref<sm::BXScene> current, next;
    double t { 0.0 };
}; // Transition

struct SceneMachine : public godot::Control 
{
    GDCLASS(SceneMachine, godot::Control)

private:

    godot::Ref<godot::PackedScene> initial_scene { nullptr };
    BXScene* current_scene { nullptr };

    std::stack<BXScene*> stack;
    
    /* 
       if there are no scenes on the stack, then the "previous scene" is the current scene
       otherwise previous is the top of the stack
    */
    BXScene* get_previous_scene() const { return stack.empty() ? current_scene : stack.top(); }

    BXScene* instantiate_scene(godot::Ref<godot::PackedScene> p_scene)
    {
        if( !p_scene.is_valid() )
        {
            godot::print_error("[SceneMachine::instantiate_scene] p_scene is not valid! ignoring ...");
            return nullptr;
        }
        
        godot::Node* scene = p_scene->instantiate();
        
        sm::BXScene* bxscene = godot::Node::cast_to<sm::BXScene>(scene);
        if( !bxscene )
        {
            godot::print_error("[SceneMachine::enter_scene] attempted to enter into scene, but it is not of type sm::BXScene! ignoring ...");
            
            scene->queue_free();
            return nullptr;
        }
        
        return bxscene;
    }

public:
    void _ready() override
    {
        // mount the initial scene
        if( initial_scene.is_valid() ) enter_scene(initial_scene);
        else godot::print_line("[SceneMachine::_ready] no initial scene. nothing to do!");
    }
    
    /*
        sets p_scene as the current scene
        enter_scene will call exit_scene for you
    */
    void enter_scene(godot::Ref<godot::PackedScene> p_scene)
    {
        // instantiate scene
        BXScene* bxscene = instantiate_scene(p_scene);
        if( !bxscene ) { godot::print_error("[SceneMachine::enter_scene] failed to instantiate p_scene! ignoring ..."); return; }
        
        // exit current scene if we haven't already
        if( current_scene != nullptr ) exit_scene();
        
        // and finally, enter new scene!
        bxscene->set_machine(this);
        add_child(bxscene);
        bxscene->enter();
        current_scene = bxscene;
        
        //godot::print_line("[SceneMachine::enter_scene] bxscene '" + bxscene->name + "' is now the current scene!");
    }
    
    /*
        exits the current scene and sets it to nullptr
    */
    void exit_scene()
    {
        if( !current_scene ) return;
        
        while( !stack.empty() )
        {
            stack.top()->exit();
            stack.pop();
        }
        
        godot::StringName& name = current_scene->name;
        current_scene->exit();
        current_scene = nullptr;
        
        //godot::print_line("[SceneMachine::exit_scene] bxscene '" + name + "' exited!");
    }
    
    /*
        pushes p_scene to the top of stack
        
        by default push_scene will keep the previous stack scene visible but paused
        to hide the previous stack scene, set visible=false
        to not pause the previous stack scene, set pause=false
    */
    void push_scene(godot::Ref<godot::PackedScene> p_scene, bool visible=true, bool pause=true)
    {
        if( !current_scene ) return;

        // instantiate scene
        BXScene* bxscene = instantiate_scene(p_scene);
        if( !bxscene ) { godot::print_error("[SceneMachine::push_scene] failed to instantiate p_scene! ignoring ..."); return; }
        
        // handle previous scene
        BXScene* previous_scene = get_previous_scene();
        if( previous_scene != nullptr )
        {
            if( pause ) previous_scene->pause(visible);
            else previous_scene->set_visible(visible);
        }

        // attach new scene
        add_child(bxscene); // attach new scene as a child of the SceneMachine (not the scene that spawned it!)
        stack.push(bxscene);
        bxscene->set_machine(this);
        bxscene->enter();
    }
    
    /* removes the top stack scene */
    void pop_scene()
    {
        if( stack.empty() ) return;
        
        stack.top()->exit(); // calls queue_free() 
        stack.pop();
        
        BXScene* previous_scene = get_previous_scene();
        if( previous_scene != nullptr ) previous_scene->resume();
    }
    
    /* GETTERS & SETTERS */

    godot::Ref<godot::PackedScene> get_initial_scene() const { return initial_scene; }
    void set_initial_scene(const godot::Ref<godot::PackedScene>& p_initial_scene) { initial_scene = p_initial_scene; }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("get_initial_scene"), &rhythm::sm::SceneMachine::get_initial_scene);
        godot::ClassDB::bind_method(godot::D_METHOD("set_initial_scene", "p_initial_scene"), &rhythm::sm::SceneMachine::set_initial_scene);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "initial_scene", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_initial_scene", "get_initial_scene");
    }
}; // SceneMachine

} // rhythm::sm
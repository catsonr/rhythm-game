#pragma once

/*
   SceneMachine is a finite state machine where each possible state is a rhythm::sm::BXScene, and
   each BXScene is itself just a godot::Control
   
   state is controlled with enter_scene() and exit_scene()
   
   SceneMachine, on top of its BXScene current_scene, has a stack<BXScene> stack, making this a pushdown automata.
   all that to say:
   SceneMachine can assume a single state. on top of that state, you may push and pop additional states as you wish
   this is useful for options menus, HUDs, shaders for Transitions, etc
   
   stack is controlled with push_scene() and pop_scene()
*/

#include <stack>

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "BXScene.h"

namespace rhythm::sm
{

/*
    Transition represents shared state between current_scene and next_scene
*/
struct Transition
{
    Transition() = delete;
    Transition(BXScene* next_scene) : next_scene(next_scene) {}

    BXScene* next_scene { nullptr };

    double t { 0.0 };
    static constexpr double t_start { 0.0 };
    static constexpr double t_end   { 1.0 };

    /* flags !*/

    // to keep current_scene alive after the Transition, set push_current_scene to false
    // once the Transition finishes, this will set next_scene to current_scene like normal, as well
    // as pushing the current scene to the stack
    bool push_current_scene = false;
    // if push_current_scene is true, then we push with these settings
    // there is probably a better way of doing this, but this works for now
    bool push_current_scene_visible = true;
    bool push_current_scene_processing = true;
    bool push_current_scene_input = false;

    // returns true if the transition is finished
    bool finished() const { return t >= t_end; }
    // restarts the Transition
    void restart() { t = t_start; }
    // interpolate by delta, where delta is in seconds
    // called every frame by SceneMachine
    void process(double delta)
    {
        if(finished()) return;

        t += delta;
        if( t > t_end ) t = t_end;
    }
}; // Transition

struct SceneMachine : public godot::Control 
{
    GDCLASS(SceneMachine, godot::Control)

public:
    Transition trans { nullptr };

private:
    godot::Ref<godot::PackedScene> initial_scene { nullptr };
    BXScene* current_scene { nullptr };

    std::stack<BXScene*> stack;

    /*
        lemma

        SceneMachine is transitioning if its Transition member has next_scene set
    */
    bool transitioning() const { return trans.next_scene != nullptr; }
    
    /* 
        lemma

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
        // tell the scene machine (and all thus its children) to take the entire screen by default
        set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

        // mount the initial scene
        if( initial_scene.is_valid() ) enter_scene(initial_scene);
        else godot::print_line("[SceneMachine::_ready] no initial scene. nothing to do!");
    }

    void _process(double delta) override
    {
        if( !transitioning() ) return;

        // process the transition
        trans.process(delta);

        // run the scenes
        current_scene->transition_out(trans);
        trans.next_scene->transition_in(trans);

        if( trans.finished() ) transition_finish();
    }
    
    /*
        sets p_scene as the current scene

        will call exit_scene for you
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
        
        godot::StringName name = current_scene->bxname();
        current_scene->exit();
        current_scene = nullptr;
    }
    
    /*
       begins a Transition to p_scene

       by default, p_scene will be drawn on top of (and processed after) current_scene. to reverse
       this, set behind=true
    */
    void transition_scene(godot::Ref<godot::PackedScene> p_scene, bool behind=false)
    {
        if( !current_scene ) return;

        BXScene* next_scene = nullptr;

        if( transitioning() )
        {
            next_scene = trans.next_scene;
            godot::print_line("[SceneMachine::transition_scene] already transitioning to '" + next_scene->bxname() + "'! restarting current transition ...");
            
            trans.restart();
            return;
        }
        else
        {
            next_scene = instantiate_scene(p_scene);
            if( !next_scene ) { godot::print_error("[SceneMachine::transition_scene] failed to instantiate p_scene!"); return; }
            
            trans = Transition( next_scene );
        }
        
        next_scene->set_machine(this);
        add_child(next_scene);
        next_scene->enter(true, true, false); // when a scene is transitioning it's visible, processing, but NO input!
        if( behind ) move_child(next_scene, current_scene->get_index());
        
        godot::print_line("[SceneMachine::transition_scene] transitioning from '" + current_scene->bxname() + "' to '" + next_scene->bxname() + "' ...");
    }
    
    /*
        exits current_scene and replaces it with next_scene, completing the transition!
        
        this can be used to prematurely finish a transition
    */
    void transition_finish()
    {
        if( !transitioning() ) return;
        
        if( trans.push_current_scene )
        {
            stack.push(current_scene);
            current_scene->enter(trans.push_current_scene_visible, trans.push_current_scene_processing, trans.push_current_scene_input);
        }
        else exit_scene();

        current_scene = trans.next_scene;
        current_scene->enter(); // enter fully (now with input)
        trans = Transition(nullptr);
        
        godot::print_line("[SceneMachine::transition_finish] transitioned to '" + current_scene->bxname() + "'!");
    }
    
    /*
        pushes a new p_scene to the top of stack
        
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
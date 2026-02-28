#pragma once

/*
   BXScene represents a state that the SceneMachine can be in
   BXScene is simply a godot::Control with 8 additional methods:

   enter,
   exit,

   pause,
   resume,

   transition_in,
   transition_out,

   set_machine, and
   get_machine
   
   enter and exit are called when SceneMachine assumes and leaves the state, respectively
   pause and resume are for when other scenes are pushed and popped from the pushdown stack, respectively

   and transition_in and transition_out are umimplemented
   
   there is also set_machine, which is called by whichever SceneMachine created the scene, where
   that machine passes itself
   get_machine is a static method that takes a godot::Node* and returns a SceneMachine*. it is a helper
   function that finds the owner of the Node (which should be a BXScene), and returns that BXScene's machine
*/

#include <godot_cpp/classes/control.hpp>

namespace rhythm::sm
{

struct SceneMachine; // forward declare for BXScene
struct Transition;   // forward declare for BXScene

struct BXScene : public godot::Control
{
    GDCLASS(BXScene, godot::Control)

public:
    SceneMachine* sm { nullptr }; // this will remain null for the main machine

    godot::StringName name { "unnamed BXScene" };
    
    void set_machine(SceneMachine* scene_machine)
    {
        if( sm != nullptr )
        {
            godot::print_error("[BXScene::set_scene_machine] bxscene '" + name + "' tried to set a scene machine, but already had one set! ignoring ...");
            return;
        }
        
        sm = scene_machine;
        
        //godot::print_line("[BXScene::set_scene_machine] '" + name + "' now owned by a scene machine!");
    }
    
    static SceneMachine* get_machine(godot::Node* node)
    {
        if( !node )
        {
            godot::print_error("[SceneMachine::get_machine] cannot get machine of null node!");
            return nullptr;
        }
        
        godot::Node* current = node;
        while( current != nullptr )
        {
            BXScene* bxscene = godot::Object::cast_to<BXScene>( current );
            if( bxscene && bxscene->sm != nullptr ) return bxscene->sm; // found a BXScene!
            
            current = current->get_parent();
        }
        
        godot::print_error("[BXScene::get_machine] failed to find a parent BXScene ...");
        return nullptr;
    }

    void enter()
    {
        set_visible(true);
        set_process_mode(godot::Node::PROCESS_MODE_INHERIT);
        
        //godot::print_line("[BXScene::enter] '" + name + "' visible and processing!");
    }
    
    void exit()
    {
        set_process(false);
        set_process_input(false);
        set_process_unhandled_input(false);
        queue_free();

        //godot::print_line("[BXScene::exit] '" + name + "' queued to free");
    }
    
    void pause(bool visible=false)
    {
        set_visible(visible);
        set_process_mode(godot::Node::PROCESS_MODE_DISABLED); // this pauses the scene and all of its children

        //godot::print_line("[BXScene::pause] '" + name + "' paused" + (visible ? " (but still visible!)" : ""));
    }
    
    void resume()
    {
        set_visible(true);
        set_process_mode(godot::Node::PROCESS_MODE_INHERIT);

        //godot::print_line("[BXScene::resume] '" + name + "' resumed, visible and processing!");
    }
    
    void transition_in(const Transition& trans);
    void transition_out(const Transition& trans);

protected:
    static void _bind_methods() {}
}; // BXScene

} // rhythm::sm
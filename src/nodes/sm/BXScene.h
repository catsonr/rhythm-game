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

   for your BXScene to have special behavior during transitions, override the transition_in and/or transition_out methods
   
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

    // a BXScene is visible and processing by default
    // to handle input, set input=true
    bool visible { true };
    bool processing { true };
    bool input { false };

public:
    SceneMachine* sm { nullptr }; // this will remain null for the main machine

    // every BXScene must override this to set its name
    // eg:
    //  godot::StringName bxname() const override { return "my cool scene!"; }
    virtual godot::StringName bxname() const { return "unnamed bxscene"; }
    
    void set_machine(SceneMachine* scene_machine)
    {
        if( sm != nullptr ) return;
        sm = scene_machine;
    }
    
    static SceneMachine* get_machine(godot::Node* node)
    {
        if( !node ) { godot::print_error("[SceneMachine::get_machine] cannot get machine of null node!"); return nullptr; }
        
        // 
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

    /*
       if visible is true, godot will render this BXScene, given it's in the viewport
       if processing is true, godot will call _process() for BXScene and its children
       if input is true, godot will pass all input to the BXScene
    */
    void enter(bool visible=true, bool processing=true, bool input=true)
    {
        set_visible(visible);

        set_process(processing);
        set_process_input(processing && input);
        set_process_unhandled_input(processing && input);
        set_process_unhandled_key_input(processing && input);
    }
    
    /*
       disables and hides the BXScene, as well as queue_free
    */
    void exit()
    {
        set_visible(false);

        set_process(false);
        set_process_input(false);
        set_process_unhandled_input(false);
        set_process_unhandled_key_input(false);
        queue_free();
    }
    
    void pause(bool visible=false)
    {
        set_visible(visible);
        set_process_mode(godot::Node::PROCESS_MODE_DISABLED); // this pauses the scene and all of its children
    }
    
    void resume()
    {
        set_visible(true);
        set_process_mode(godot::Node::PROCESS_MODE_INHERIT); // this plays the scene unless its parent is paused
    }
    
    virtual void transition_in(const Transition& trans) {}
    virtual void transition_out(const Transition& trans) {}

protected:
    static void _bind_methods() {}
}; // BXScene

} // rhythm::sm
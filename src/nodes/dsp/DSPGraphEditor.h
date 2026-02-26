#pragma once

#include <godot_cpp/classes/control.hpp>

#include "DSPGraphEdit.h"

namespace rhythm::dsp
{

/*

struct DSPGraphEditor : public godot::Control
{
    GDCLASS(DSPGraphEditor, godot::Control)

private:
    //dsp::DSPGraphEdit* dspgraphedit;

public:
    void _ready() override
    {
        // WARN: all of this is commented out because in its current state,
        // libbeatboxx will crash if DSPGraphEditor::_ready() is called, since DSPGraphEdit tries to
        // Scene::conjure_ctx() before its parent (this!) has FINISHED _ready()) which means that it 
        // is not yet owned by a rhythm::Scene!
        // dspgraphedit = memnew(dsp::DSPGraphEdit);
        // add_child(dspgraphedit);
    }

protected:
    static void _bind_methods() {}
}; // DSPGraphEditor

*/

} // rhythm::dsp
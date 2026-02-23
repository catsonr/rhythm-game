#pragma once

#include "ma_dsp_godot.h"

#include <godot_cpp/classes/label.hpp>

namespace rhythm::dsp
{

struct OutputNode : public DSPNode
{
    ma_node* endpoint = nullptr;
    
    ma_result init(ma_engine* p_engine) override
    {
        endpoint = (ma_node*)ma_engine_get_endpoint(p_engine);
        
        return MA_SUCCESS;
    }
    
    ma_node* get_ma_node() override { return endpoint; }
}; // OutputNode

struct OutputGraphNode : public DSPGraphNode
{
    GDCLASS(OutputGraphNode, DSPGraphNode)

public:
    void _ready() override
    {
        set_title("output");
        set_slot(0, true, 0, dsp::in_color, false, 0, dsp::out_color);

        godot::Label* label = memnew(godot::Label);
        label->set_text("in");
        add_child(label);
    }

protected:
    static void _bind_methods() {}
}; // OutputGraphNode

} // rhythm::dsp
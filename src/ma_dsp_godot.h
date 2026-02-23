#pragma once

#include <godot_cpp/classes/graph_node.hpp>

#include "miniaudio.h"

namespace rhythm::dsp
{

static godot::Color in_color  { 1, 1, 1, 1 };
static godot::Color out_color { 1, 1, 0, 1 };

struct DSPNode
{
    virtual ~DSPNode() = default;
    virtual ma_result init(ma_engine* p_engine) = 0;
    virtual ma_node* get_ma_node() = 0;
}; // DSPNode

struct DSPGraphNode : godot::GraphNode
{
    GDCLASS(DSPGraphNode, godot::GraphNode)

protected:
    DSPNode* dsp_node = nullptr;

public:
    virtual DSPNode* get_dsp_node() const { return dsp_node; }
    virtual void set_dsp_node(DSPNode* p_node ) { dsp_node = p_node; }
    
    // turns the graph node red if there is no dsp node set
    void _draw() override
    {
        if( !dsp_node )
        {
            godot::Vector2 size = get_size();
            
            draw_rect({ 0, 0, size.x, size.y }, { 1, 0, 0, 1 });
        }
    }

protected:
    static void _bind_methods() {}
}; // DSPGraphNode

} // rhythm::dsp
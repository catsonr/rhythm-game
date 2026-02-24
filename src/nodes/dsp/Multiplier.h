#pragma once

#include <atomic>

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

#include "ma_dsp_godot.h"

namespace rhythm::dsp
{

struct MultiplierNode; // forward declare for multiplier_node

struct multiplier_node
{
    ma_node_base base;
    MultiplierNode* parent;
    
    static void process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);
    
    static inline ma_node_vtable vtable { process, nullptr, 2, 1, MA_NODE_FLAG_CONTINUOUS_PROCESSING };
}; // multiplier_node

struct MultiplierNode : public DSPNode
{
    multiplier_node node;
    
    std::atomic<double> multiplier { 100.0 };
    
    ma_result init(ma_engine* p_engine) override
    {
        ma_uint32 channels = ma_engine_get_channels(p_engine);
        
        node.parent = this;
        
        ma_uint32 input_channels[2] { channels, channels };
        ma_uint32 output_channels[1] { channels };
        
        ma_node_config node_config = ma_node_config_init();
        node_config.vtable = &multiplier_node::vtable;
        node_config.pInputChannels = input_channels;
        node_config.pOutputChannels = output_channels;
        
        ma_node_graph* graph = ma_engine_get_node_graph(p_engine);
        ma_result result = ma_node_init(graph, &node_config, nullptr, &node.base);
        if( result != MA_SUCCESS ) return result;

        return ma_node_set_state(&node.base, ma_node_state_started);
    }
    
    ma_node* get_ma_node() override { return &node.base; }
    
    void set_multiplier(const double p_multiplier)
    {
        multiplier.store( p_multiplier, std::memory_order_relaxed );
    }
}; // MultiplierNode

struct MultiplierGraphNode : public DSPGraphNode
{
    GDCLASS(MultiplierGraphNode, DSPGraphNode)

private:
    godot::HSlider* multiplier_slider { nullptr };
    godot::Label* multiplier_label { nullptr };

public:
    void _ready() override
    {
        set_title("multiplier");
        
        set_slot(0, true, 0, dsp::in_color, true, 0, dsp::out_color);
        godot::Label* label = memnew(godot::Label);
        label->set_text("in / out");
        add_child(label);
        
        set_slot(1, true, 0, dsp::in_color,false, 0, dsp::out_color);
        godot::HBoxContainer* multiplier_hboxcontainer = memnew(godot::HBoxContainer);
        multiplier_slider = memnew(godot::HSlider);
        multiplier_slider->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        multiplier_slider->set_custom_minimum_size({60, 0});
        multiplier_slider->set_min(-200);
        multiplier_slider->set_max(200);
        multiplier_slider->set_step(1);
        multiplier_slider->connect("value_changed", callable_mp(this, &MultiplierGraphNode::multiplier_slider_value_changed));
        multiplier_hboxcontainer->add_child(multiplier_slider);
        
        multiplier_label = memnew(godot::Label);
        multiplier_hboxcontainer->add_child(multiplier_label);
        
        add_child(multiplier_hboxcontainer);
    }
    
    void _process(double delta) override
    {
        if( !dsp_node ) return;

        const double multiplier = ((MultiplierNode*)dsp_node)->multiplier.load(std::memory_order_relaxed);
        multiplier_slider->set_value_no_signal(multiplier);
        multiplier_label->set_text(godot::String::num( multiplier, 1 ));
    }
    
    void multiplier_slider_value_changed(double value)
    {
        if( !dsp_node ) return;
        ((MultiplierNode*)dsp_node)->set_multiplier(value);
    }
    
protected:
    static void _bind_methods() {}
}; // MultiplierGraphNode

inline void multiplier_node::process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
{
    MultiplierNode* dsp_node = ((multiplier_node*)pNode)->parent;
    
    const float* in = ppFramesIn[0];
    float* out = ppFramesOut[0];
    
    const float* multiplier_in = ppFramesIn[1];
    
    const ma_uint32 channels = 2;

    const double current_multiplier = dsp_node->multiplier.load(std::memory_order_relaxed);
    
    if( in && multiplier_in )
        for(ma_uint32 i = 0; i < *pFrameCountOut * channels; i++)
            out[i] = (in[i] + multiplier_in[i]) * current_multiplier;
    else if( in )
        for(ma_uint32 i = 0; i < *pFrameCountOut * channels; i++)
            out[i] = in[i] * current_multiplier;
    else
        for(ma_uint32 i = 0; i < *pFrameCountOut * channels; i++)
            out[i] = 0;
}

} // rhythm::dsp
#pragma once

#include "ma_dsp_godot.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/option_button.hpp>

namespace rhythm::dsp
{

struct OscillatorNode; // forward declare for oscillator_node

struct oscillator_node
{
    ma_node_base base;
    OscillatorNode* parent;
    
    static void process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);
    
    static inline ma_node_vtable vtable { process, nullptr, 2, 1, MA_NODE_FLAG_CONTINUOUS_PROCESSING };
}; // oscillator_node

struct OscillatorNode : public DSPNode
{
    ma_waveform waveform;
    oscillator_node node;
    
    ma_waveform_type type { ma_waveform_type_sine };
    double frequency { 440.0 }; // INPUT 0
    double amplitude { 0.5 }; // INPUT 1
    
    ma_result init(ma_engine* p_engine) override
    {
        ma_uint32 channels = ma_engine_get_channels(p_engine);
        ma_uint32 sample_rate = ma_engine_get_sample_rate(p_engine);
        
        ma_waveform_config waveform_config = ma_waveform_config_init(
            ma_format_f32, channels, sample_rate, type, amplitude, frequency
        );
        ma_result result = ma_waveform_init(&waveform_config, &waveform);
        if( result != MA_SUCCESS ) return result;
        
        node.parent = this;
        ma_uint32 input_channels[2] { channels, channels };
        ma_uint32 output_channels[1] { channels };
        
        ma_node_config node_config = ma_node_config_init();
        node_config.vtable = &oscillator_node::vtable;
        node_config.pInputChannels = input_channels;
        node_config.pOutputChannels = output_channels;
        
        ma_node_graph* graph = ma_engine_get_node_graph(p_engine);
        result = ma_node_init(graph, &node_config, nullptr, &node.base);
        if( result != MA_SUCCESS ) return result;

        return ma_node_set_state(&node.base, ma_node_state_started);
    }
    
    ma_node* get_ma_node() override { return &node.base; }
    
    void set_type(int p_type_index)
    {
        type = (ma_waveform_type)p_type_index;
        ma_waveform_set_type(&waveform, type);
    }
    void set_frequency(double p_frequency)
    {
        frequency = p_frequency;
        ma_waveform_set_frequency(&waveform, frequency);
    }
    void set_amplitude(double p_amplitude)
    {
        amplitude = p_amplitude;
        ma_waveform_set_amplitude(&waveform, amplitude);
    }
}; // OscillatorNode

struct OscillatorGraphNode : public DSPGraphNode
{
    GDCLASS(OscillatorGraphNode, DSPGraphNode)

private:
    godot::OptionButton* type_option_button { nullptr };
    godot::HSlider* frequency_slider { nullptr };
    godot::Label* frequency_label { nullptr };
    godot::HSlider* amplitude_slider { nullptr };
    godot::Label* amplitude_label { nullptr };

public:
    void _ready() override
    {
        set_title("oscillator");

        // in / out
        set_slot(0, false, 0, dsp::in_color, true, 0, dsp::out_color);
        
        type_option_button = memnew(godot::OptionButton);
        type_option_button->add_item("sine", ma_waveform_type_sine);
        type_option_button->add_item("square", ma_waveform_type_square);
        type_option_button->add_item("triangle", ma_waveform_type_triangle);
        type_option_button->add_item("sawtooth", ma_waveform_type_sawtooth);
        type_option_button->connect("item_selected", callable_mp(this, &OscillatorGraphNode::type_option_button_item_selected));
        add_child(type_option_button);
        
        set_slot(1, true, 0, dsp::in_color, false, 0, dsp::out_color);
        godot::HBoxContainer* frequency_hboxcontainer = memnew(godot::HBoxContainer);
        frequency_slider = memnew(godot::HSlider);
        frequency_slider->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        frequency_slider->set_min(0.0);
        frequency_slider->set_max(2000.0);
        frequency_slider->set_exp_ratio(true);
        frequency_slider->connect("value_changed", callable_mp(this, &OscillatorGraphNode::frequency_slider_value_changed));
        frequency_hboxcontainer->add_child(frequency_slider);

        frequency_label = memnew(godot::Label);
        frequency_label->set_text("idk");
        frequency_hboxcontainer->add_child(frequency_label);

        add_child(frequency_hboxcontainer);

        set_slot(2, true, 0, dsp::in_color, false, 0, dsp::out_color);
        godot::HBoxContainer* amplitude_hboxcontainer = memnew(godot::HBoxContainer);
        amplitude_slider = memnew(godot::HSlider);
        amplitude_slider->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        amplitude_slider->set_min(0.0);
        amplitude_slider->set_max(1.0);
        amplitude_slider->set_step(0.01);
        amplitude_slider->connect("value_changed", callable_mp(this, &OscillatorGraphNode::amplitude_slider_value_changed));
        amplitude_hboxcontainer->add_child(amplitude_slider);
        
        amplitude_label = memnew(godot::Label);
        amplitude_label->set_text("idk");
        amplitude_hboxcontainer->add_child(amplitude_label);

        add_child(amplitude_hboxcontainer);
    }
    
    void _process(double delta) override
    {
        if( !dsp_node ) return;
        
        const double frequency = ((OscillatorNode*)dsp_node)->frequency;
        frequency_slider->set_value(frequency);
        frequency_label->set_text(godot::String::num_real( frequency ));

        const double amplitude = ((OscillatorNode*)dsp_node)->amplitude;
        amplitude_slider->set_value(amplitude);
        amplitude_label->set_text(godot::String::num_real( amplitude ));
    }
    
    void type_option_button_item_selected(int index)
    {
        if( !dsp_node ) return;
        ((OscillatorNode*)dsp_node)->set_type(index);
    }
    
    void frequency_slider_value_changed(double value)
    {
        if( !dsp_node ) return;
        
        ((OscillatorNode*)dsp_node)->set_frequency(value);
    }
    
    void amplitude_slider_value_changed(double value)
    {
        if( !dsp_node ) return;
        
        ((OscillatorNode*)dsp_node)->set_amplitude(value);
    }

protected:
    static void _bind_methods() {} // not being inherited from dsp::DSPGraphNode (??)
}; // OscillatorGraphNode

inline void oscillator_node::process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
{
    oscillator_node* ma_node = (oscillator_node*)pNode;
    OscillatorNode* dsp_node = ma_node->parent;
    
    float* out = ppFramesOut[0];
    const float* frequency_in = ppFramesIn[0];
    const float* amplitude_in = ppFramesIn[1];
    
    const ma_uint32 channels = 2;
    
    // add frequency input
    for(ma_uint32 i = 0; i < *pFrameCountOut; i++)
    {
        // ignores R channel of frequency (intended behavior)
        if( frequency_in )
            ma_waveform_set_frequency(&dsp_node->waveform, dsp_node->frequency + frequency_in[i*channels]*100.); 

        ma_waveform_read_pcm_frames(&dsp_node->waveform, &out[i*channels], 1, nullptr);
        
        if( amplitude_in )
            for(int c = 0; c < channels; c++)
                out[i*channels + c] *= (1.0 + amplitude_in[i*channels + c]);
    }
    
}

} // rhythm::dsp
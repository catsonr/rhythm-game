#pragma once

#include "ma_dsp_godot.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/option_button.hpp>

namespace rhythm::dsp
{

struct OscillatorNode : public DSPNode
{
    ma_waveform waveform;
    ma_data_source_node node;
    
    ma_waveform_type type { ma_waveform_type_sine };
    double frequency { 440.0 };
    double amplitude { 0.5 };
    
    ma_result init(ma_engine* p_engine) override
    {
        ma_uint32 channels = ma_engine_get_channels(p_engine);
        ma_uint32 sample_rate = ma_engine_get_sample_rate(p_engine);
        
        ma_waveform_config waveform_config = ma_waveform_config_init(
            ma_format_f32, channels, sample_rate, type, amplitude, frequency
        );
        ma_result result = ma_waveform_init(&waveform_config, &waveform);
        if( result != MA_SUCCESS ) return result;
        
        ma_node_graph* graph = ma_engine_get_node_graph(p_engine);
        ma_data_source_node_config data_source_node_config = ma_data_source_node_config_init(&waveform);
        result = ma_data_source_node_init(graph, &data_source_node_config, nullptr, &node);
        if( result != MA_SUCCESS ) return result;

        return ma_node_set_state(&node, ma_node_state_started);
    }
    
    ma_node* get_ma_node() override { return &node; }
    
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
    godot::HSlider* amplitude_slider { nullptr };

public:
    void _ready() override
    {
        set_title("oscillator");
        set_slot(0, false, 0, dsp::in_color, true, 0, dsp::out_color);
        
        type_option_button = memnew(godot::OptionButton);
        type_option_button->add_item("sine", ma_waveform_type_sine);
        type_option_button->add_item("square", ma_waveform_type_square);
        type_option_button->add_item("triangle", ma_waveform_type_triangle);
        type_option_button->add_item("sawtooth", ma_waveform_type_sawtooth);
        type_option_button->connect("item_selected", callable_mp(this, &OscillatorGraphNode::type_option_button_item_selected));
        add_child(type_option_button);
        
        frequency_slider = memnew(godot::HSlider);
        frequency_slider->set_min(20.0);
        frequency_slider->set_max(20000.0);
        frequency_slider->set_exp_ratio(true);
        frequency_slider->connect("value_changed", callable_mp(this, &OscillatorGraphNode::frequency_slider_value_changed));
        add_child(frequency_slider);

        amplitude_slider = memnew(godot::HSlider);
        amplitude_slider->set_min(0.0);
        amplitude_slider->set_max(1.0);
        amplitude_slider->set_step(0.01);
        amplitude_slider->connect("value_changed", callable_mp(this, &OscillatorGraphNode::amplitude_slider_value_changed));
        add_child(amplitude_slider);
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

} // rhythm::dsp
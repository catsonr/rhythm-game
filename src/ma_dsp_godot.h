#pragma once

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/spin_box.hpp>

#include "miniaudio.h"

#include "biquad_config.h"

namespace rhythm::dsp
{

struct OscillatorNode
{
    ma_data_source_node node;

    ma_waveform oscillator;
    ma_waveform_type waveform_type = ma_waveform_type_triangle;
    double amplitude = 0.5;
    
    ma_result init(ma_engine* p_engine)
    {
        ma_waveform_config waveform_config = ma_waveform_config_init(ma_format_f32, 2, p_engine->sampleRate, waveform_type, amplitude, 440.0);
        ma_waveform_init(&waveform_config, &oscillator);

        ma_data_source_node_config oscillator_node_config = ma_data_source_node_config_init(&oscillator);
        return ma_data_source_node_init(ma_engine_get_node_graph(p_engine), &oscillator_node_config, nullptr, &node);
    }

    struct OscillatorGraphNode : public godot::GraphNode
    {
        GDCLASS(OscillatorGraphNode, godot::GraphNode)

    private:
        OscillatorNode* oscillator_node { nullptr };
        
    public:
        void _ready() override
        {
            set_title("oscillator");
            
            godot::Label* label = memnew(godot::Label);
            label->set_text("out");
            add_child(label);
            
            set_slot(0, false, 0, godot::Color(1, 0, 1), true, 0, godot::Color(0, 1, 1));
        }
        
        OscillatorNode* get_oscillator_node() const { return oscillator_node; }
        void set_oscillator_node(OscillatorNode* p_oscillator_node) { oscillator_node = p_oscillator_node; }
    protected:
        static void _bind_methods() {}
    }; // OscillatorGraphNode
}; // OscillatorNode 

struct EnvelopeNode
{
    ma_node_base node;

    uint32_t sample_rate;

    uint64_t attack_frames = 41000;
    uint64_t decay_frames = 4000;
    double sustain_amplitude = 0.8; // [0.0, 1.0]
    uint64_t release_frames = 20500;
    
    enum struct State { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = State::IDLE;
    
    double current_amplitude = 0.0;
    double target_amplitude = 0.0;
    double damplitude_dframe = 0.0;
    
    ma_result init(ma_engine* p_engine)
    {
        sample_rate = p_engine->sampleRate;
        ma_uint32 channels = 2;
        
        ma_node_config node_config = ma_node_config_init();
        node_config.vtable = &vtable;
        node_config.pInputChannels = &channels;
        node_config.pOutputChannels = &channels;
        
        return ma_node_init(ma_engine_get_node_graph(p_engine), &node_config, nullptr, &node);
    }

    static void process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        EnvelopeNode* envelope_node = (EnvelopeNode*)pNode;

        const float* input = ppFramesIn[0];
        float* output = ppFramesOut[0];
        ma_uint32 frames = pFrameCountIn[0];
        
        for(ma_uint32 i = 0; i < frames; i++)
        {
            double amplitude = envelope_node->tick();
            
            output[i*2 + 0] = input[i*2 + 0] * amplitude;
            output[i*2 + 1] = input[i*2 + 1] * amplitude;
        }

        *pFrameCountOut = frames;
    }
    
    double tick()
    {
        if( state == State::IDLE || state == State::SUSTAIN ) return current_amplitude;
        
        current_amplitude += damplitude_dframe;
        
        bool passed_going_up   = (damplitude_dframe > 0.0 && current_amplitude >= target_amplitude);
        bool passed_going_down = (damplitude_dframe < 0.0 && current_amplitude <= target_amplitude);
        if( passed_going_up || passed_going_down )
        {
            current_amplitude = target_amplitude;
            advance_state();
        }
        
        return current_amplitude;
    }
    
    void advance_state()
    {
        if( state == State::ATTACK )
        {
            state = State::DECAY;
            target_amplitude = sustain_amplitude;
            if( decay_frames > 0 )
            {
                damplitude_dframe = (target_amplitude - current_amplitude) / decay_frames;
            }
            else
            {
                current_amplitude = target_amplitude;
                advance_state();
                return;
            }
        }
        else if( state == State::DECAY )
        {
            state = State::SUSTAIN;
            damplitude_dframe = 0.0;
        }
        else if( state == State::RELEASE )
        {
            state = State::IDLE;
            current_amplitude = 0.0;
            damplitude_dframe = 0.0;
        }
    }
    
    void play()
    {
        state = State::ATTACK;
        target_amplitude = 1.0;

        if( attack_frames > 0 ) damplitude_dframe = (target_amplitude - current_amplitude) / attack_frames;
        else
        {
            current_amplitude = target_amplitude;
            advance_state();
        }
    }

    void stop() {
        if( state != State::IDLE && state != State::RELEASE )
        {
            state = State::RELEASE;
            target_amplitude = 0.0;
            
            if( release_frames > 0 ) damplitude_dframe = (target_amplitude - current_amplitude) / release_frames;
            else
            {
                current_amplitude = 0.0;
                state = State::IDLE;
                damplitude_dframe = 0.0;
            }
        }
    }

    static inline ma_node_vtable vtable { process, nullptr, 1, 1, MA_NODE_FLAG_CONTINUOUS_PROCESSING }; 
    
    struct EnvelopeGraphNode : public godot::GraphNode
    {
        GDCLASS(EnvelopeGraphNode, godot::GraphNode)
    
    private:
        EnvelopeNode* envelope_node { nullptr };
    
    public:
        void _ready() override
        {
            set_title("envelope");
            
            godot::Label* label = memnew(godot::Label);
            label->set_text("in / out");
            add_child(label);
            
            set_slot(0, true, 0, godot::Color(1, 0, 1), true, 0, godot::Color(0, 1, 1));
        }
        
        void _process(double delta) override
        {
            queue_redraw();
        }
        
        void _draw() override
        {
            godot::Vector2 size = get_size();
            
            if( envelope_node ) draw_rect({ 0, 0, size.x, size.y }, { 0, 0, static_cast<real_t>(envelope_node->current_amplitude), 1});
        }

        EnvelopeNode* get_envelope_node() const { return envelope_node; }
        void set_envelope_node(EnvelopeNode* p_envelope_node) { envelope_node = p_envelope_node; }
    
    protected:
        static void _bind_methods() {}
    }; // EnvelopeGraphNode
}; // EnvelopeNode

struct FilterNode
{
    ma_node_base node;
    
    uint32_t sample_rate;

    ma_biquad biquad;

    rhythm::dsp::bqcfg::Type type = rhythm::dsp::bqcfg::Type::biquad;
    
    // following miniaudio parameters
    double cutoffFrequency = 1000;
    uint32_t order = 2;
    double q = 0.707;
    double gainDB = 0;
    double frequency = 880.0; // ?
    double shelfSlope = 1;
    
    ma_result init(ma_engine* p_engine)
    {
        sample_rate = p_engine->sampleRate;
        ma_uint32 channels = 2;
        
        ma_node_config node_config = ma_node_config_init();
        node_config.vtable = &vtable;
        node_config.pInputChannels = &channels;
        node_config.pOutputChannels = &channels;
        ma_result result = ma_node_init(ma_engine_get_node_graph(p_engine), &node_config, nullptr, &node);
        if( result != MA_SUCCESS ) return result;
        
        ma_biquad_config biquad_config = ma_biquad_config_init(ma_format_f32, channels, 1, 0, 0, 1, 0, 0);
        ma_biquad_init(&biquad_config, nullptr, &biquad);
        
        set_coefficients();
        return MA_SUCCESS;
    }
    
    void set_coefficients()
    {
        ma_biquad_config config;
        ma_uint32 channels = 2;
        
        // see biquad_config.h
        switch( type )
        {
            case rhythm::dsp::bqcfg::Type::lpf2:
            {
                ma_lpf2_config lpf2_config = ma_lpf2_config_init(ma_format_f32, channels, sample_rate, cutoffFrequency, q);
                config = rhythm::dsp::bqcfg::lpf2(&lpf2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::hpf2:
            {
                ma_hpf2_config hpf2_config = ma_hpf2_config_init(ma_format_f32, channels, sample_rate, cutoffFrequency, q);
                config = rhythm::dsp::bqcfg::hpf2(&hpf2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::bpf2:
            {
                ma_bpf2_config bpf2_config = ma_bpf2_config_init(ma_format_f32, channels, sample_rate, cutoffFrequency, q);
                config = rhythm::dsp::bqcfg::bpf2(&bpf2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::notch2:
            {
                ma_notch2_config notch2_config = ma_notch2_config_init(ma_format_f32, channels, sample_rate, q, frequency);
                config = rhythm::dsp::bqcfg::notch2(&notch2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::peak2:
            {
                ma_peak2_config peak2_config = ma_peak2_config_init(ma_format_f32, channels, sample_rate, gainDB, q, frequency);
                config = rhythm::dsp::bqcfg::peak2(&peak2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::loshelf2:
            {
                ma_loshelf2_config loshelf2_config = ma_loshelf2_config_init(ma_format_f32, channels, sample_rate, gainDB, shelfSlope, q);
                config = rhythm::dsp::bqcfg::loshelf2(&loshelf2_config);
                break;
            }
            case rhythm::dsp::bqcfg::Type::hishelf2:
            {
                ma_hishelf2_config hishelf2_config = ma_hishelf2_config_init(ma_format_f32, channels, sample_rate, gainDB, shelfSlope, q);
                config = rhythm::dsp::bqcfg::hishelf2(&hishelf2_config);
                break;
            }
            
            default: break;
        }
        
        ma_biquad_reinit(&config, &biquad);
    }
    
    static void process(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        FilterNode* filter_node = (FilterNode*)pNode;
        
        ma_biquad_process_pcm_frames(&filter_node->biquad, ppFramesOut[0], ppFramesIn[0], *pFrameCountIn);
        
        *pFrameCountOut = *pFrameCountIn;
    }
    
    static inline ma_node_vtable vtable { process, nullptr, 1, 1, 0 };
    
    struct FilterGraphNode : public godot::GraphNode
    {
        GDCLASS(FilterGraphNode, godot::GraphNode)
            
    private:
        FilterNode* filter_node;

        godot::Label* coefficients_label;
        godot::OptionButton* type_option_button;

        godot::HSlider* cutoffFrequency_slider;
        godot::OptionButton* order_option_button;
        godot::HSlider* q_slider;
        godot::HSlider* gainDB_slider;
        godot::HSlider* frequency_slider;
        godot::HSlider* shelfSlope_slider;
    
    public:
        void _ready() override
        {
            set_title("filter");
            
            godot::Label* label = memnew(godot::Label);
            label->set_text("in / out");
            add_child(label);
            
            coefficients_label = memnew(godot::Label);
            add_child(coefficients_label);
            
            type_option_button = memnew(godot::OptionButton);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::biquad), (int)bqcfg::Type::biquad);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::lpf2), (int)bqcfg::Type::lpf2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::hpf2), (int)bqcfg::Type::hpf2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::bpf2), (int)bqcfg::Type::bpf2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::notch2), (int)bqcfg::Type::notch2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::peak2), (int)bqcfg::Type::peak2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::loshelf2), (int)bqcfg::Type::loshelf2);
            type_option_button->add_item(bqcfg::type_to_string(bqcfg::Type::hishelf2), (int)bqcfg::Type::hishelf2);
            type_option_button->connect("item_selected", callable_mp(this, &FilterGraphNode::type_option_button_item_selected));
            add_child(type_option_button);
            
            cutoffFrequency_slider = memnew(godot::HSlider);
            cutoffFrequency_slider->set_tooltip_text("cutoffFrequency");
            cutoffFrequency_slider->set_min(20);
            cutoffFrequency_slider->set_max(20000);
            cutoffFrequency_slider->set_step(1);
            cutoffFrequency_slider->set_value(22);
            cutoffFrequency_slider->set_exp_ratio(true);
            cutoffFrequency_slider->connect("value_changed", callable_mp(this, &FilterGraphNode::cutoffFrequency_slider_value_changed));
            add_child(cutoffFrequency_slider);
            
            /* it seems like this is never even used by miniaudio ??
            order_option_button = memnew(godot::OptionButton);
            order_option_button->add_item("1st order", 1);
            order_option_button->add_item("2nd order", 2);
            order_option_button->add_item("4th order", 4);
            order_option_button->connect("item_selected", callable_mp(this, &FilterGraphNode::order_option_button_item_selected));
            add_child(order_option_button);
            */
            
            q_slider = memnew(godot::HSlider);
            q_slider->set_tooltip_text("resonance (q)");
            q_slider->set_min(0.1);
            q_slider->set_max(10);
            q_slider->set_step(0.01);
            q_slider->set_value(0.2);
            q_slider->connect("value_changed", callable_mp(this, &FilterGraphNode::q_slider_value_changed));
            add_child(q_slider);
            
            gainDB_slider = memnew(godot::HSlider);
            gainDB_slider->set_tooltip_text("gain (dB)");
            gainDB_slider->set_min(-24);
            gainDB_slider->set_max(24);
            gainDB_slider->set_step(0.05);
            gainDB_slider->set_value(-2);
            gainDB_slider->connect("value_changed", callable_mp(this, &FilterGraphNode::gainDB_slider_value_changed));
            add_child(gainDB_slider);

            frequency_slider = memnew(godot::HSlider);
            frequency_slider->set_tooltip_text("frequency");
            frequency_slider->set_min(20);
            frequency_slider->set_max(20000);
            frequency_slider->set_step(1);
            frequency_slider->set_value(22);
            frequency_slider->set_exp_ratio(true);
            frequency_slider->connect("value_changed", callable_mp(this, &FilterGraphNode::frequency_slider_value_changed));
            add_child(frequency_slider);

            shelfSlope_slider = memnew(godot::HSlider);
            shelfSlope_slider->set_tooltip_text("shelf slope");
            shelfSlope_slider->set_min(-5);
            shelfSlope_slider->set_max(5);
            shelfSlope_slider->set_step(0.01);
            shelfSlope_slider->set_value(2.2);
            shelfSlope_slider->connect("value_changed", callable_mp(this, &FilterGraphNode::shelfSlope_slider_value_changed));
            add_child(shelfSlope_slider);
            
            set_slot(0, true, 0, godot::Color(1, 0, 1), true, 0, godot::Color(0, 1, 1));
        }
        
        void _process(double delta) override
        {
            if( filter_node )
            {
                ma_biquad& biquad = filter_node->biquad;
                coefficients_label->set_text(
                    "b0: " + godot::String::num_real(biquad.b0.f32) +
                    " b1: " + godot::String::num_real(biquad.b1.f32) +
                    " b2: " + godot::String::num_real(biquad.b2.f32) +
                    " a0: 1.0" + // miniaudio normalizes all coeffs w a0 (i.e., a0 always 1.0)
                    " a1: " + godot::String::num_real(biquad.a1.f32) +
                    " a2: " + godot::String::num_real(biquad.a2.f32)
                );
            }
            else coefficients_label->set_text("no filter node set!");
        }
        
        void type_option_button_item_selected(int index)
        {
            if( !filter_node ) return;
            
            filter_node->type = static_cast<bqcfg::Type>( index );
            filter_node->set_coefficients();
        }
        
        void cutoffFrequency_slider_value_changed(double value)
        {
            if( !filter_node ) return;

            filter_node->cutoffFrequency = value;
            filter_node->set_coefficients();
        }
        
        void order_option_button_item_selected(int index)
        {
            if( !filter_node ) return;

            filter_node->order = index;
            filter_node->set_coefficients();
        }
        
        void q_slider_value_changed(double value)
        {
            if( !filter_node ) return;

            filter_node->q = value;
            filter_node->set_coefficients();
        }
        
        void gainDB_slider_value_changed(double value)
        {
            if( !filter_node ) return;
            
            filter_node->gainDB = value;
            filter_node->set_coefficients();
        }
        
        void frequency_slider_value_changed(double value)
        {
            if( !filter_node ) return;

            filter_node->frequency = value;
            filter_node->set_coefficients();
        }
        
        void shelfSlope_slider_value_changed(double value)
        {
            if( !filter_node ) return;

            filter_node->shelfSlope = value;
            filter_node->set_coefficients();
        }
        
        FilterNode* get_filter_node() const { return filter_node; }
        void set_filter_node(FilterNode* p_filter_node) { filter_node = p_filter_node; }

    protected:
        static void _bind_methods() {}
    }; // FilterGraphNode
}; // FilterNode

struct OutputGraphNode : public godot::GraphNode
{
    GDCLASS(OutputGraphNode, godot::GraphNode)

public:
    void _ready() override
    {
        set_title("output");

        godot::Label* label = memnew(godot::Label);
        label->set_text("in");
        add_child(label);
        
        set_slot(0, true, 0, godot::Color(1, 0, 1), false, 0, godot::Color(0, 1, 1));
    }

protected:
    static void _bind_methods() {}
}; // OutputGraphNode

} // rhythm::dsp

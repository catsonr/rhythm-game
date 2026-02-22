#pragma once

#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/label.hpp>

#include "miniaudio.h"

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
            
            set_slot(0, false, 0, godot::Color(1, 1, 1), true, 0, godot::Color(1, 1, 0));
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

    static inline ma_node_vtable vtable { process, nullptr, 1, 1, 0 }; 
    
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
            
            set_slot(0, true, 0, godot::Color(1, 1, 0), true, 0, godot::Color(1, 1, 0));
        }

        EnvelopeNode* get_envelope_node() const { return envelope_node; }
        void set_envelope_node(EnvelopeNode* p_envelope_node) { envelope_node = p_envelope_node; }
    
    protected:
        static void _bind_methods() {}
    }; // EnvelopeGraphNode
}; // EnvelopeNode

struct Voice
{
    OscillatorNode oscillator;
    EnvelopeNode envelope;

    ma_result init(ma_engine* p_engine)
    {
        oscillator.init(p_engine);
        envelope.init(p_engine);
        
        return MA_SUCCESS;
    }
}; // Voice

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
        
        set_slot(0, true, 0, godot::Color(1, 1, 0), false, 0, godot::Color(1, 1, 1));
    }

protected:
    static void _bind_methods() {}
}; // OutputGraphNode

} // rhythm::dsp

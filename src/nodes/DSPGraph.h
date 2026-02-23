#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/graph_edit.hpp>

#include "SceneManager.h"
#include "ma_dsp_godot.h"

namespace rhythm
{

struct DSPGraph : public godot::GraphEdit
{
    GDCLASS(DSPGraph, GraphEdit)

private:
    rhythm::dsp::EnvelopeNode envelope;
    rhythm::dsp::FilterNode filter;
    rhythm::dsp::OscillatorNode oscillator;

public:
    void _ready() override
    {
        set_right_disconnects(true);
        set_minimap_enabled(false);
        set_show_arrange_button(false);
        set_show_grid_buttons(false);

        connect("connection_request", godot::Callable(this, "on_connection_request"));
        connect("disconnection_request", godot::Callable(this, "on_disconnection_request"));
        
        ma_engine& engine = Scene::conjure_ctx(this)->audio_engine_2->engine;
        envelope.init(&engine);
        filter.init(&engine);
        oscillator.init(&engine);
        
        godot::TypedArray<godot::Node> children = get_children();
        for(int i = 0; i < children.size(); i++)
        {
            godot::Node* child = godot::Object::cast_to<godot::Node>(children[i]);
            
            rhythm::dsp::OscillatorNode::OscillatorGraphNode* osc = godot::Object::cast_to<rhythm::dsp::OscillatorNode::OscillatorGraphNode>(child);
            if( osc ) osc->set_oscillator_node(&oscillator);

            rhythm::dsp::EnvelopeNode::EnvelopeGraphNode* env = godot::Object::cast_to<rhythm::dsp::EnvelopeNode::EnvelopeGraphNode>(child);
            if( env ) env->set_envelope_node(&envelope);

            rhythm::dsp::FilterNode::FilterGraphNode* fil = godot::Object::cast_to<rhythm::dsp::FilterNode::FilterGraphNode>(child);
            if( fil ) fil->set_filter_node(&filter);
        }
    }

    void _input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_SPACE:
                {
                    envelope.play();
                    break;
                }
                
                default: break;
            }
        }
        if( key_event.is_valid() && key_event->is_released() )
        {
            switch( key_event->get_physical_keycode() )
            {
                case godot::KEY_SPACE:
                {
                    envelope.stop();
                    break;
                }
                default: break;
            }
        }
    }
    
    void _draw() override
    {
        godot::Vector2 size = get_size();

        draw_rect({ 0, 0, size.x, size.y }, {0, 0, 0, 0.5});
    }
    
    ma_node* get_ma_node(const godot::StringName& node_name)
    {
        godot::Node* node = get_node<godot::Node>(godot::NodePath(node_name));
        if( !node ) return nullptr;

        rhythm::dsp::OscillatorNode::OscillatorGraphNode* osc = godot::Object::cast_to<rhythm::dsp::OscillatorNode::OscillatorGraphNode>(node);
        if( osc ) return (ma_node*)&osc->get_oscillator_node()->node;

        rhythm::dsp::EnvelopeNode::EnvelopeGraphNode* env = godot::Object::cast_to<rhythm::dsp::EnvelopeNode::EnvelopeGraphNode>(node);
        if( env ) return (ma_node*)&env->get_envelope_node()->node;

        rhythm::dsp::FilterNode::FilterGraphNode* fil = godot::Object::cast_to<rhythm::dsp::FilterNode::FilterGraphNode>(node);
        if( fil ) return (ma_node*)&fil->get_filter_node()->node;

        rhythm::dsp::OutputGraphNode* out = godot::Object::cast_to<rhythm::dsp::OutputGraphNode>(node);
        if( out ) return ma_engine_get_endpoint( &Scene::conjure_ctx(this)->audio_engine_2->engine );
        
        return nullptr;
    }
    
    void on_connection_request(const godot::StringName& from_node, int from_port, const godot::StringName& to_node, int to_port)
    {
        ma_node* source = get_ma_node(from_node);
        ma_node* dest   = get_ma_node(to_node);
        
        if( source && dest )
        {
            ma_node_attach_output_bus(source, from_port, dest, to_port);
            connect_node(from_node, from_port, to_node, to_port);
        }
    }
    void on_disconnection_request(const godot::StringName& from_node, int from_port, const godot::StringName& to_node, int to_port)
    {
        ma_node* source = get_ma_node(from_node);

        if( source )
        {
            ma_node_detach_output_bus(source, from_port);
            disconnect_node(from_node, from_port, to_node, to_port);
        }
    }

protected:
    static void _bind_methods()
    {
        godot::ClassDB::bind_method(godot::D_METHOD("on_connection_request", "from_node", "from_port", "to_node", "to_port"), &DSPGraph::on_connection_request);
        godot::ClassDB::bind_method(godot::D_METHOD("on_disconnection_request", "from_node", "from_port", "to_node", "to_port"), &DSPGraph::on_disconnection_request);
    }
}; // DSPGraph

} // rhythm
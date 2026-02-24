#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/graph_edit.hpp>

#include "SceneManager.h"
#include "ma_dsp_godot.h"

#include "Oscillator.h"
#include "Output.h"

namespace rhythm::dsp
{

struct DSPGraph : public godot::GraphEdit
{
    GDCLASS(DSPGraph, GraphEdit)

private:
    rhythm::dsp::OscillatorNode oscillator_node;
    rhythm::dsp::OscillatorGraphNode* oscillator_graph_node { nullptr };

    rhythm::dsp::OscillatorNode oscillator_node_2;
    rhythm::dsp::OscillatorGraphNode* oscillator_graph_node_2 { nullptr };

    rhythm::dsp::OutputNode output_node;
    rhythm::dsp::OutputGraphNode* output_graph_node { nullptr };

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
        
        oscillator_node_2.init(&engine);
        oscillator_graph_node_2 = memnew(rhythm::dsp::OscillatorGraphNode);
        oscillator_graph_node_2->set_dsp_node(&oscillator_node_2);
        add_child(oscillator_graph_node_2);

        oscillator_node.init(&engine);
        oscillator_graph_node = memnew(rhythm::dsp::OscillatorGraphNode);
        oscillator_graph_node->set_dsp_node(&oscillator_node);
        add_child(oscillator_graph_node);
        
        output_node.init(&engine);
        output_graph_node = memnew(rhythm::dsp::OutputGraphNode);
        output_graph_node->set_dsp_node(&output_node);
        add_child(output_graph_node);
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
                    //envelope.play();
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
                    //envelope.stop();
                    break;
                }
                default: break;
            }
        }
    }
    
    ma_node* get_ma_node(const godot::StringName& node_name)
    {
        godot::Node* node = get_node_or_null(godot::NodePath(node_name));
        if( !node )
        {
            godot::print_error("[DSPGraph::get_ma_node] node '" + node_name + "' is null!");
            return nullptr;
        }
        
        rhythm::dsp::DSPGraphNode* graph_node = godot::Object::cast_to<rhythm::dsp::DSPGraphNode>(node);
        if( !graph_node )
        {
            godot::print_error("[DSPGraph::get_ma_node] node '" + node_name + "' is not a rhythm::dsp::DSPGraphNode!");
            return nullptr;
        }
        
        rhythm::dsp::DSPNode* dsp_node = graph_node->get_dsp_node();
        if( !dsp_node )
        {
            godot::print_error("[DSPGraph::get_ma_node] node '" + node_name + "' is a rhythm::dsp::DSPGraphNode, but has no rhythm::dsp::DSPNode dsp_node!");
            return nullptr;
        }
        
        ma_node* ma_node = dsp_node->get_ma_node();
        if( !ma_node )
        {
            godot::print_error("[DSPGraph::get_ma_node] node '" + node_name + "' has a rhythm::dsp::DSPNode dsp_node, but dsp_node has no ma_node!");
            return nullptr;
        }
        
        return ma_node;
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

} // rhythm::dsp
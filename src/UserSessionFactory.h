#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>

#include "SceneManager.h"

namespace rhythm
{

struct UserSessionFactory : public godot::Control
{
    GDCLASS(UserSessionFactory, Control)

private:
    BXApi* bxapi { nullptr };
    godot::VBoxContainer* login_vbox { nullptr };
    godot::LineEdit* username_lineedit { nullptr };
    godot::LineEdit* password_lineedit { nullptr };
    godot::Button* login_button { nullptr };

public:
    void _ready() override
    {
        bxapi = Scene::conjure_ctx(this)->bxapi;

        login_vbox = memnew(godot::VBoxContainer);
        add_child(login_vbox);
        
        username_lineedit = memnew(godot::LineEdit);
        username_lineedit->set_placeholder("username");
        username_lineedit->set_anchors_and_offsets_preset(godot::Control::LayoutPreset::PRESET_VCENTER_WIDE);
        login_vbox->add_child(username_lineedit);

        password_lineedit = memnew(godot::LineEdit);
        password_lineedit->set_placeholder("password");
        login_vbox->add_child(password_lineedit);
        
        login_button = memnew(godot::Button);
        login_button->set_text("log in!");
        login_button->connect("pressed", callable_mp(this, &UserSessionFactory::login_button_pressed));
        login_vbox->add_child(login_button);
    }

    void _draw() override
    {
        godot::Vector2 size = get_size();
        float w = size.x;
        float h = size.y;
        
        draw_rect({ 0, 0, w, h }, { 0, 0, 0, 0.5 });
    }

    void login_button_pressed()
    {
        godot::String username = username_lineedit->get_text();
        godot::String password = password_lineedit->get_text();
        
        bxapi->ping();
    }
    
    /* GETTERS & SETTERS */

protected:
    static void _bind_methods() {}
}; // UserSessionFactory

} // rhythm
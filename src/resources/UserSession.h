#pragma once

#include <godot_cpp/classes/resource.hpp>

//#include "../bbxxserver/models/models.h"

namespace rhythm
{

struct UserSession : public godot::Resource
{
    GDCLASS(UserSession, Resource);
    
private:
    godot::StringName uuid;
    
    /* GETTERS & SETTERS */
public:
    godot::StringName get_uuid() const { return uuid; }
    void set_uuid(const godot::StringName& p_uuid) { uuid = p_uuid; }
    
protected:
    static void _bind_methods()
    {
        // uuid
        godot::ClassDB::bind_method(godot::D_METHOD("get_uuid"), &rhythm::UserSession::get_uuid);
        godot::ClassDB::bind_method(godot::D_METHOD("set_uuid", "p_uuid"), &rhythm::UserSession::set_uuid);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "uuid"), "set_uuid", "get_uuid");
    }
}; // UserSession

} // rhythm


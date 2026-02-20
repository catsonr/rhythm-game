#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace rhythm
{

struct Album : public godot::Resource
{
    GDCLASS(Album, Resource);

    godot::StringName title;
    godot::StringName artist;
    int release_year { 0 };
    godot::Ref<godot::Texture2D> cover;

    static void _bind_methods()
    {
        // title
        godot::ClassDB::bind_method(godot::D_METHOD("get_title"), &rhythm::Album::get_title);
        godot::ClassDB::bind_method(godot::D_METHOD("set_title", "p_title"), &rhythm::Album::set_title);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "title"), "set_title", "get_title");

        // artist
        godot::ClassDB::bind_method(godot::D_METHOD("get_artist"), &rhythm::Album::get_artist);
        godot::ClassDB::bind_method(godot::D_METHOD("set_artist", "p_artist"), &rhythm::Album::set_artist);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "artist"), "set_artist", "get_artist");

        // release_year
        godot::ClassDB::bind_method(godot::D_METHOD("get_release_year"), &rhythm::Album::get_release_year);
        godot::ClassDB::bind_method(godot::D_METHOD("set_release_year", "p_release_year"), &rhythm::Album::set_release_year);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "release_year"), "set_release_year", "get_release_year");

        // cover
        godot::ClassDB::bind_method(godot::D_METHOD("get_cover"), &rhythm::Album::get_cover);
        godot::ClassDB::bind_method(godot::D_METHOD("set_cover", "p_cover"), &rhythm::Album::set_cover);
        ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cover", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_cover", "get_cover");
    }
    
    /* GETTERS & SETTERS */
    
public:
    godot::StringName get_title() const { return title; }
    void set_title(const godot::StringName& p_title) { title = p_title; }
    
    godot::StringName get_artist() const { return artist; }
    void set_artist(const godot::StringName& p_artist) { artist = p_artist; }
    
    int get_release_year() const { return release_year; }
    void set_release_year(int p_release_year) { release_year = p_release_year; }
    
    godot::Ref<godot::Texture2D> get_cover() const { return cover; }
    void set_cover(const godot::Ref<godot::Texture2D>& p_cover ) { cover = p_cover; }
    
    
}; // Album

} // rhythm
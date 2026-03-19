#pragma once

#include <filesystem>
#include <unordered_set>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/font.hpp>

#include "nodes/sm/BXScene.h"

namespace fs = std::filesystem;

struct Row
{
    Row() = delete;
    Row(const fs::path& path, int depth) :
        path(path),
        depth(depth)
    {
        text = path.filename().string().c_str();
        is_folder = fs::is_directory(path);
        if( is_folder ) text += "/";
    }

    fs::path path;
    int depth;

    godot::String text;
    bool is_folder;
}; // Row

struct Folder
{
    Folder() = delete;
    Folder(const fs::path& path) : path(path) {}

    fs::path path;

    std::vector<Folder> children;
    std::vector<fs::path> files;
    
    /* lemmas */
    
    bool sourced() const { return !children.empty() || !files.empty(); }

    static bool is_audio_file(const fs::path& p)
    {
        static const std::unordered_set<std::string> EXTENSIONS { ".mp3", ".wav", ".flac", ".ogg" };
        
        return EXTENSIONS.count( p.extension().string() );
    }
    
    std::vector<Row> rows(int depth=0) const
    {
        std::vector<Row> rows;
        rows.emplace_back( path, depth );
        
        for( const Folder& child : children )
        {
            std::vector<Row> child_rows = child.rows(depth+1);
            rows.insert(rows.end(), child_rows.begin(), child_rows.end());
        }
        for( const fs::path& path : files )
        {
            rows.emplace_back( path, depth+1 );
        }
        
        return rows;
    }
    
    /* operations */
    
    void source()
    {
        children.clear();
        files.clear();
        
        if( !fs::exists(path) || !fs::is_directory(path) ) return;
        
        for( const fs::directory_entry& entry : fs::directory_iterator(path) )
        {
            if( entry.is_directory() )
            {
                Folder child { entry.path() };
                child.source();

                if( child.sourced() ) children.push_back(std::move(child));
            }
            else if( entry.is_regular_file() && is_audio_file(entry.path()) ) files.push_back(entry.path());
        }
    }
}; // Folder

namespace rhythm::sm
{

struct Fingerprinter : public sm::BXScene
{
    GDCLASS(Fingerprinter, sm::BXScene)

private:
    std::unordered_set<std::string> known_folders;
    std::vector<Folder> folders;
    std::vector<Row> rows;
    
    godot::Button* button { nullptr };
    godot::FileDialog* file_dialog { nullptr };

public:
    godot::StringName bxname() const override { return "Fingerprinter"; }
    
    void _ready() override
    {
        set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);

        button = memnew(godot::Button);
        button->set_anchors_and_offsets_preset(godot::Control::PRESET_CENTER_BOTTOM, godot::Control::PRESET_MODE_MINSIZE, 15);
        button->set_text("+");
        button->connect("pressed", callable_mp(this, &Fingerprinter::on_pressed));
        add_child(button);

        file_dialog = memnew(godot::FileDialog);
        file_dialog->set_file_mode(godot::FileDialog::FILE_MODE_OPEN_DIR);
        file_dialog->set_access(godot::FileDialog::ACCESS_FILESYSTEM);
        file_dialog->connect("dir_selected", callable_mp(this, &Fingerprinter::on_dir_selected));
        add_child(file_dialog);
    }

    void _unhandled_input(const godot::Ref<godot::InputEvent>& event) override
    {
        godot::Ref<godot::InputEventKey> key_event = event;
        if( key_event.is_valid() && key_event->is_pressed() && !key_event->is_echo() )
        {
            switch(key_event->get_physical_keycode())
            {
                case godot::KEY_ESCAPE:
                {
                    SM_TRANSITION(observatory)
                    break;
                }
                default: break;
            }
        }
    }
    
    void _process(double delta) override
    {
        queue_redraw();
    }
    
    void _draw() override
    {

        godot::Ref<godot::Font> font = get_theme_default_font();
        int font_size = get_theme_default_font_size();

        const float padding = 4;
        const float tab = 8;
        float row_height = font_size;
        
        for( int i = 0; i < (int)rows.size(); i++ )
        {
            const Row& row = rows[i];
            
            float y = padding + row_height*(i+1);
            float x = padding + row.depth*tab;
            
            draw_string(font, { x, y }, row.text, godot::HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, {1,1,1,1});
        }
    }
    
    void on_pressed()
    {
        file_dialog->popup_centered({ 700, 500 });
    }
    
    void on_dir_selected(const godot::String& dir)
    {
        fs::path path { dir.utf8().get_data() };
        if( known_folders.count(path.string()) ) return;

        Folder folder { path };

        folder.source();
        if( !folder.sourced() ) return;

        known_folders.insert( path.string() );
        folders.push_back( std::move(folder) );
        rows = generate_rows();
    }

    std::vector<Row> generate_rows() const
    {
        std::vector<Row> rows;
        for( const Folder& folder : folders )
        {
            std::vector<Row> r = folder.rows();
            rows.insert(rows.end(), r.begin(), r.end());
        }
        
        return rows;
    }

protected:
    static void _bind_methods() {}
}; // Fingerprinter

} // rhythm::sm
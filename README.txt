i followed (https://docs.godotengine.org/en/4.4/tutorials/scripting/gdextension/gdextension_cpp_example.html)

'gdexample.gdextension' is from the tutorial, quote "
    Using the GDExtension module
    Before we jump back into Godot, we need to create one more file in demo/bin/.

    This file lets Godot know what dynamic libraries should be loaded for each platform and the entry function for the module. It is called gdexample.gdextension.
    
    This file contains a configuration section that controls the entry function of the module. You should also set the minimum compatible Godot version with compatibility_minimum, which prevents older version of Godot from trying to load your extension. The reloadable flag enables automatic reloading of your extension by the editor every time you recompile it, without needing to restart the editor. This only works if you compile your extension in debug mode (default).
"

compile with command 'scons', defined by 'Sconstruct' file, which will output create 'demo/'

so after compiling you should have a demo/bin/ directory containing a .framework/.so directory
copy that directory AND gdexample.gdextension into my-cool-project/bin/

this can be automatically done with
sh build.sh path/to/project


# ASSUMPTIONS
 - AudioEngine is assumed to be an autoloaded singleton, and also that it exists
 - Run is assumed to have a child Sprite2D named 'BG'
 - there exists a file "res://audio/midnight.mp3"
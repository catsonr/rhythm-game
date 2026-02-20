#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

lib_name = "libbeatboxx"
out_dir = os.path.join("rhythm-game", "bin")

env.Append(CPPPATH=["src/", "src/nodes/", "src/resources"])
sources = Glob("src/*.cpp") + Glob("src/nodes/*.cpp") + Glob("src/resources/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{dir}/{lib}.{plt}.{tgt}.framework/{lib}.{plt}.{tgt}".format(
            dir=out_dir, lib=lib_name, plt=env["platform"], tgt=env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{}/{}{}{}".format(out_dir, lib_name, env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

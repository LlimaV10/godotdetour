#!/usr/bin/env python

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=[
    "src/",
    "src/util/",
    "recastnavigation/DebugUtils/Include/",
    "recastnavigation/Detour/Include/",
    "recastnavigation/DetourCrowd/Include/",
    "recastnavigation/DetourTileCache/Include/",
    "recastnavigation/Recast/Include/",
])

sources = Glob("src/*.cpp")
sources += Glob("src/util/*.cpp")
sources += Glob("src/util/*.c")
sources += Glob("recastnavigation/DebugUtils/Source/*.cpp")
sources += Glob("recastnavigation/Detour/Source/*.cpp")
sources += Glob("recastnavigation/DetourCrowd/Source/*.cpp")
sources += Glob("recastnavigation/DetourTileCache/Source/*.cpp")
sources += Glob("recastnavigation/Recast/Source/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/addons/godotdetour/bin/libgodotdetour.{}.{}".format(env["platform"], env["target"]),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/addons/godotdetour/bin/libgodotdetour.{}.{}.simulator".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/addons/godotdetour/bin/libgodotdetour.{}.{}".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/addons/godotdetour/bin/libgodotdetour{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

env.NoCache(library)
Default(library)

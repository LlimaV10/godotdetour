Godot 4 addon packaging

Use `godotdetour.gdextension` in Godot 4 projects.

Current packaged binary:
- windows.debug.x86_64

To use this in another Godot 4 project:
1. Copy the entire `addons/godotdetour` folder.
2. In the target project, use `res://addons/godotdetour/godotdetour.gdextension`.
3. Instantiate the registered classes directly in GDScript, for example:
   - `var nav = DetourNavigation.new()`
   - `var params = DetourNavigationParameters.new()`

Notes:
- The old `.gdnlib` / `.gdns` files are Godot 3 artifacts and should not be used in a Godot 4 project.
- If you need a release build, run:
  `py -3 -m SCons platform=windows target=template_release -j1`
- If you need Linux/macOS binaries, build those targets and add them to `godotdetour.gdextension`.

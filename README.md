# godotdetour

> Experimental fork warning
>
> This repository is currently an experimental fork. It contains ongoing Godot 4 / GDExtension migration work and other changes that may be incomplete or broken.
>
> Expect rough edges, regressions, missing features, and API/behavior changes while the fork is being stabilized.

Godot 4 GDExtension fork of the original `godotdetour` project, using [recastnavigation](https://github.com/recastnavigation/recastnavigation) for 3D navigation meshes, agents, dynamic obstacles, and crowds.

![demo2](https://media.giphy.com/media/YT8OWY266iqbGRNeWc/source.gif)

**Status:** Experimental Godot 4 fork

This repository is no longer in the original Godot 3 / GDNative state described by upstream documentation. The current fork has been moved toward:

- Godot 4
- GDExtension / `godot-cpp`
- newer `recastnavigation`
- a working but still evolving demo and addon packaging

Do not assume old README instructions from the original project still apply.

## Current state

What currently works in this fork:

- native build for Godot 4 on Windows
- GDExtension packaging in `demo/addons/godotdetour`
- working Godot 4 demo project
- runtime navmesh generation from a `MeshInstance3D`
- crowd agents
- temporary obstacles
- convex area marking and selective tile rebuilds
- off-mesh connections
- chunk-based source geometry inside fixed horizontal nav bounds
- threaded navigation updates
- external-position agent mode, where Godot can own actual movement and the addon provides steering velocity

What is still rough or incomplete:

- the fork is not production-hardened yet
- packaged binaries are currently limited
- some legacy Godot 3 files are still present in the addon folder as leftovers
- some old signals are bound but not currently emitted in the runtime path
- documentation from the original project is outdated unless explicitly updated for this fork
- expanding navmesh bounds at runtime is not supported; chunk updates must stay inside the horizontal bounds frozen during initialization

## Important differences from the original project

This fork is not the old Godot 3 GDNative plugin anymore.

- It is now a Godot 4 GDExtension-based addon.
- The old `.gdnlib` and `.gdns` files in the addon directory are legacy leftovers and should not be used for new Godot 4 projects.
- The correct Godot 4 entry file is `demo/addons/godotdetour/godotdetour.gdextension`.
- The working demo and current usage reference are the files under `demo/`, especially `demo/main.gd`.

## Building

The current verified build path in this fork is Windows + Godot 4 debug.

1. Check out the repo
2. Initialize submodules:

```
git submodule update --init --recursive
```

3. Build the extension:

```powershell
python -m SCons platform=windows target=template_debug -j1
```

Current output:

- `demo/addons/godotdetour/bin/libgodotdetour.windows.template_debug.x86_64.dll`

If you need release or other platforms, build those targets and update `demo/addons/godotdetour/godotdetour.gdextension` accordingly.

## Demo

Open the Godot 4 project under `demo/`.

The demo currently shows:

- Godot 4 GDExtension loading
- navmesh initialization from a `MeshInstance3D`
- debug mesh rendering
- crowd agent creation and retargeting
- threaded updates

## Using the addon in a Godot 4 project

Copy:

- `demo/addons/godotdetour`

into your own Godot 4 project as:

- `res://addons/godotdetour`

Use:

- `res://addons/godotdetour/godotdetour.gdextension`

Instantiate the registered classes directly:

```gdscript
var navigation := DetourNavigation.new()
var nav_params := DetourNavigationParameters.new()
var mesh_params := DetourNavigationMeshParameters.new()
var agent_params := DetourCrowdAgentParameters.new()
```

For full addon usage documentation, see:

- `demo/addons/godotdetour/README.md`

## Current API notes

- `DetourNavigation.initialize(mesh_instance, params)` builds from a single `MeshInstance3D` or an `Array[MeshInstance3D]`.
- Internally the navmesh is tiled.
- Partial tile rebuilds currently exist for:
  - source-geometry chunk changes inside the original horizontal nav bounds
  - convex area markers
  - off-mesh connections
  - dynamic obstacles
- Runtime chunk helpers now exist:
  - `addSourceGeometryChunk()`
  - `updateSourceGeometryChunk()`
  - `removeSourceGeometryChunk()`
  - `getSourceGeometryChunkIDs()`
- Chunk edits must remain within the horizontal nav bounds established during `initialize()`. Vertical growth is allowed, but a larger X/Z world footprint still requires reinitialization.

Supported area types used by the current fork:

- `0`: ground
- `1`: water
- `2`: road
- `3`: door
- `4`: grass
- `5`: jump

## Movement modes

The current fork supports two agent movement styles:

- Simulated position mode:
  - the addon updates agent position itself
- External position mode:
  - the addon computes steering / desired velocity
  - Godot moves the actual actor
  - your code feeds the resulting position back with `agent.syncPosition()`

## Documentation lookup support

This repo also contains:

- addon usage docs in `demo/addons/godotdetour/README.md`
- GDScript stubs in `demo/addons/godotdetour/stubs`

These are intended to help editor lookup and external tooling, but behavior depends on your Godot / editor setup.

## Legacy note

Much of the original README below used to describe the Godot 3 GDNative version of the project. That no longer reflects the recommended workflow for this fork and has been intentionally replaced with Godot 4-specific guidance.

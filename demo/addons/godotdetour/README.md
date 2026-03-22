# GodotDetour for Godot 4

GodotDetour is a native Godot 4 GDExtension that wraps Recast/Detour crowd navigation.

This repository is currently set up and verified for:

- Godot `4.x`
- `godot-cpp` GDExtension bindings
- RecastNavigation `1.6.0`
- Windows `x86_64` debug binary packaging

The addon registers these classes directly in Godot:

- `DetourNavigation`
- `DetourNavigationParameters`
- `DetourNavigationMeshParameters`
- `DetourCrowdAgent`
- `DetourCrowdAgentParameters`
- `DetourObstacle`

## 1. Installing the addon

Copy this folder into your Godot 4 project:

- `res://addons/godotdetour`

The Godot 4 entry file is:

- `res://addons/godotdetour/godotdetour.gdextension`

Current packaged binary:

- `res://addons/godotdetour/bin/libgodotdetour.windows.template_debug.x86_64.dll`

Important:

- The old `.gdnlib` and `.gdns` files in this folder are Godot 3 leftovers. Do not use them in Godot 4.
- The current packaged library is Windows debug only. If you need release or other platforms, build them and extend `godotdetour.gdextension`.

## 2. Verifying that the addon loaded

Use a smoke test script:

```gdscript
extends Node

func _ready() -> void:
	var nav := DetourNavigation.new()
	var nav_params := DetourNavigationParameters.new()
	var mesh_params := DetourNavigationMeshParameters.new()
	var agent_params := DetourCrowdAgentParameters.new()

	print(nav)
	print(nav_params)
	print(mesh_params)
	print(agent_params)
```

If the extension loaded correctly:

- the script parses
- the classes autocomplete in the editor
- the scene runs without "Unknown identifier" errors

## 3. High-level architecture

Typical flow:

1. Create `DetourNavigation`
2. Create `DetourNavigationParameters`
3. Create one or more `DetourNavigationMeshParameters`
4. Call `navigation.initialize(mesh_instance_3d, nav_params)`
5. Define filters with `setQueryFilter()`
6. Optionally mark areas / add off-mesh connections / add obstacles
7. Create agents with `addAgent()`
8. Move agents with `agent.moveTowards(target_position)`
9. Read `agent.position`, `agent.velocity`, `agent.isMoving`

## 4. Runtime model: threaded vs manual ticking

The addon currently supports both patterns:

- Threaded mode:
  - `initialize()` starts an internal worker thread
  - crowd simulation updates in the background
  - your scene typically only reads `agent.position` and `agent.velocity`
- Manual mode:
  - `tick(delta_seconds)` is also exposed
  - you can choose to disable threaded startup in code and drive updates yourself from `_process()` or `_physics_process()`

Current repo state:

- The demo uses the threaded mode again
- `tick()` is still available as a debugging / safer integration path

Tradeoff:

- Threaded mode reduces main-thread work, but visual motion can look stepped unless you interpolate.
- Manual `tick()` is easier to debug and reason about.

## 5. Minimal usage example

```gdscript
extends Node3D

@onready var source_mesh: MeshInstance3D = $Ground

var navigation: DetourNavigation
var agent: DetourCrowdAgent

func _ready() -> void:
	navigation = DetourNavigation.new()

	var nav_params := DetourNavigationParameters.new()
	nav_params.ticksPerSecond = 30
	nav_params.maxObstacles = 64

	var mesh_params := DetourNavigationMeshParameters.new()
	mesh_params.cellSize = Vector2(0.25, 0.2)
	mesh_params.maxNumAgents = 32
	mesh_params.maxAgentSlope = 45.0
	mesh_params.maxAgentHeight = 2.0
	mesh_params.maxAgentClimb = 0.6
	mesh_params.maxAgentRadius = 0.5
	mesh_params.maxEdgeLength = 12.0
	mesh_params.maxSimplificationError = 1.3
	mesh_params.minNumCellsPerIsland = 8
	mesh_params.minCellSpanCount = 20
	mesh_params.maxVertsPerPoly = 6
	mesh_params.tileSize = 48
	mesh_params.layersPerTile = 4
	mesh_params.detailSampleDistance = 6.0
	mesh_params.detailSampleMaxError = 1.0
	nav_params.navMeshParameters.append(mesh_params)

	if not navigation.initialize(source_mesh, nav_params):
		push_error("Navigation initialization failed")
		return

	navigation.setQueryFilter(0, "default", {
		0: 1.0,
		1: 1.0,
		2: 999999.0,
		3: 2.0,
		4: 3.0,
		5: 3.0,
	})

	var agent_params := DetourCrowdAgentParameters.new()
	agent_params.position = Vector3(0.0, 0.0, 0.0)
	agent_params.radius = 0.35
	agent_params.height = 1.7
	agent_params.maxAcceleration = 8.0
	agent_params.maxSpeed = 3.0
	agent_params.filterName = "default"
	agent_params.anticipateTurns = true
	agent_params.optimizeVisibility = true
	agent_params.optimizeTopology = true
	agent_params.avoidObstacles = true
	agent_params.avoidOtherAgents = true
	agent_params.obstacleAvoidance = 1
	agent_params.separationWeight = 1.5

	agent = navigation.addAgent(agent_params)
	if agent == null:
		push_error("Failed to create agent")
		return

	agent.moveTowards(Vector3(8.0, 0.0, 8.0))

func _process(_delta: float) -> void:
	if agent == null:
		return

	# Drive your visible model from the simulated state.
	$Actor.global_position = agent.position
```

## 6. Chunked source geometry

The current fork can now manage the source mesh as multiple chunks, but there
is one hard constraint:

- horizontal navmesh bounds are frozen during `initialize()`
- later chunk adds/updates must remain inside those original X/Z bounds
- vertical growth is allowed; chunk geometry can introduce higher or lower walkable regions later
- if you need a larger streamed world footprint, initialize with all expected
  chunks already present or reinitialize with a larger combined source

You can initialize from multiple chunks directly:

```gdscript
var chunks: Array[MeshInstance3D] = [$ChunkA, $ChunkB, $ChunkC]
navigation.initialize(chunks, nav_params)
```

At runtime you can manage chunks explicitly:

```gdscript
var chunk_id := navigation.addSourceGeometryChunk($ChunkD)
if chunk_id >= 0:
	navigation.rebuildChangedTiles()

navigation.updateSourceGeometryChunk(chunk_id, $ChunkDUpdated)
navigation.rebuildChangedTiles()

navigation.removeSourceGeometryChunk(chunk_id)
navigation.rebuildChangedTiles()
```

Useful notes:

- `getSourceGeometryChunkIDs()` returns the currently tracked chunk IDs
- chunk edits only affect the touched X/Z tile columns when `rebuildChangedTiles()` runs
- geometry chunk rebuilds regenerate full affected tile columns, which is more expensive than obstacle updates but more robust for holes and newly added elevated regions
- removing the last remaining chunk is rejected; use `clear()` if you want to tear the whole navigation instance down

## 7. DetourNavigation API

### Methods

- `initialize(input_mesh_instance: Variant, parameters: DetourNavigationParameters) -> bool`
  - Builds navmeshes from either a single `MeshInstance3D` or an `Array[MeshInstance3D]`.
- `tick(delta_seconds: float = -1.0) -> void`
  - Manually advances the simulation.
  - Mostly useful for debugging or if you want a main-thread integration model.
- `rebuildChangedTiles() -> void`
  - Rebuilds tiles after changing marked areas, off-mesh connections, or source geometry chunks.
  - Source geometry chunk changes rebuild full affected tile columns so new vertical layers can appear.
- `addSourceGeometryChunk(input_mesh_instance: Variant) -> int`
  - Adds a source-geometry chunk inside the frozen horizontal navmesh bounds and returns its chunk ID.
- `updateSourceGeometryChunk(chunk_id: int, input_mesh_instance: Variant) -> bool`
  - Replaces an existing source-geometry chunk.
- `removeSourceGeometryChunk(chunk_id: int) -> bool`
  - Removes an existing source-geometry chunk.
- `getSourceGeometryChunkIDs() -> Array`
  - Returns the tracked source chunk IDs.
- `markConvexArea(vertices: Array, height: float, area_type: int) -> int`
  - Marks a convex area on the source geometry.
- `addOffMeshConnection(from: Vector3, to: Vector3, bidirectional: bool, radius: float, area_type: int) -> int`
  - Adds an off-mesh connection.
- `removeOffMeshConnection(id: int) -> void`
- `setQueryFilter(index: int, name: String, weights: Dictionary) -> bool`
  - Defines a named query filter used by agents.
- `addAgent(parameters: DetourCrowdAgentParameters) -> DetourCrowdAgent`
- `removeAgent(agent: DetourCrowdAgent) -> void`
- `addBoxObstacle(position: Vector3, dimensions: Vector3, rotation_rad: float) -> DetourObstacle`
- `addCylinderObstacle(position: Vector3, radius: float, height: float) -> DetourObstacle`
- `createDebugMesh(index: int, draw_cache_bounds: bool) -> MeshInstance3D`
- `save(path: String, compressed: bool) -> bool`
- `load(path: String, compressed: bool) -> bool`
- `clear() -> void`
- `getAgents() -> Array`
- `getObstacles() -> Array`
- `getMarkedAreaIDs() -> Array`
- `isInitialized() -> bool`

### Signal

- `navigation_tick_done(executionTimeSeconds: float)`

Current note:

- This signal exists in the class binding, but the current code does not emit it. Do not rely on it.

## 8. DetourNavigationParameters

### Properties

- `navMeshParameters: Array`
  - Array of `DetourNavigationMeshParameters`
- `ticksPerSecond: int`
  - Update rate for threaded mode
- `maxObstacles: int`
- `defaultAreaType: int`

## 9. DetourNavigationMeshParameters

### Properties

- `cellSize: Vector2`
- `maxNumAgents: int`
- `maxAgentSlope: float`
- `maxAgentHeight: float`
- `maxAgentClimb: float`
- `maxAgentRadius: float`
- `maxEdgeLength: float`
- `maxSimplificationError: float`
- `minNumCellsPerIsland: int`
- `minCellSpanCount: int`
- `maxVertsPerPoly: int`
- `tileSize: int`
- `layersPerTile: int`
- `detailSampleDistance: float`
- `detailSampleMaxError: float`

These parameters control the generated Recast navmesh and crowd limits. The demo scene in `res://main.gd` is the best working reference for values that are currently known to behave correctly in this port.

## 10. Query filters

Agents choose a named filter through `DetourCrowdAgentParameters.filterName`.

Define filters first:

```gdscript
navigation.setQueryFilter(0, "default", {
	0: 1.0,
	1: 1.0,
	2: 999999.0,
	3: 2.0,
	4: 3.0,
	5: 3.0,
})
```

Then assign the name to each agent:

```gdscript
agent_params.filterName = "default"
```

Notes:

- A very large weight is treated as effectively excluded.
- Filters must exist before `addAgent()`.

## 10. DetourCrowdAgentParameters

### Properties

- `position: Vector3`
- `radius: float`
- `height: float`
- `maxAcceleration: float`
- `maxSpeed: float`
- `filterName: String`
- `anticipateTurns: bool`
- `optimizeVisibility: bool`
- `optimizeTopology: bool`
- `avoidObstacles: bool`
- `avoidOtherAgents: bool`
- `obstacleAvoidance: int`
- `separationWeight: float`

## 11. DetourCrowdAgent

### Methods

- `moveTowards(position: Vector3) -> void`
- `stop() -> void`
- `getPredictedMovement(current_pos: Vector3, current_dir: Vector3, position_ticks_timestamp: int, max_turning_rad: float) -> Dictionary`

### Read-only properties

- `position: Vector3`
- `velocity: Vector3`
- `target: Vector3`
- `isMoving: bool`

### Signals

- `arrived_at_target(node)`
- `no_progress(node, distanceLeft)`
- `no_movement(node)`

Current note:

- These signals are bound, but the current Godot 4 port does not emit them during runtime because signal emission from the worker update path was disabled for stability. Do not rely on them right now.

## 12. DetourObstacle

### Methods

- `move(position: Vector3) -> void`
- `destroy() -> void`

### Properties

- `position: Vector3`
- `dimensions: Vector3`

## 13. Debug mesh

To visualize the navmesh:

```gdscript
var debug_mesh := navigation.createDebugMesh(0, false)
add_child(debug_mesh)
```

Notes:

- The `index` selects which navmesh to draw.
- `draw_cache_bounds = true` also draws tile cache bounds.

## 14. Saving and loading

You can save and load the native navigation state:

```gdscript
navigation.save("user://nav.bin", true)
navigation.load("user://nav.bin", true)
```

Notes:

- Use `clear()` before loading a different dataset into an already initialized navigation object.
- Save/load support exists in the native code, but you should validate it against your game data before relying on it in production.

## 15. Threading and visual smoothness

If you use threaded mode, visible actors can look like they update in steps because the simulation runs at `ticksPerSecond`, not every rendered frame.

Options:

- Increase `ticksPerSecond`
- Interpolate your visible model between old and new agent positions
- Use `getPredictedMovement()` for client-side smoothing
- Use manual `tick()` if you prefer simpler frame-to-frame control

## 16. Current limitations of this Godot 4 port

- The Godot 4 binary packaging in this repo is currently Windows debug only.
- The old Godot 3 `.gdns` / `.gdnlib` resources are still present as leftovers, but they are not part of the Godot 4 path.
- Bound signals such as `arrived_at_target` are currently not emitted in the runtime path.
- Threaded mode is working in the demo, but the addon has not yet been hardened as a polished production plugin API.
- There is no editor plugin UI. Usage is script-driven.

## 17. Recommended integration approach

For a real project, use one of these two patterns explicitly:

- Conservative:
  - drive the addon via `tick()` from `_physics_process()`
  - simpler debugging and fewer threading risks
- Performance-oriented:
  - use the built-in threaded mode
  - add interpolation for visible models
  - avoid depending on runtime signals that are currently disabled

## 18. Demo reference

The working Godot 4 demo in this repository is the best practical reference:

- `res://main.gd`
- `res://main.tscn`

It shows:

- navmesh initialization
- query filter setup
- convex area marking
- agent creation
- retargeting agents
- drawing the debug navmesh

## 19. Building the native library

Example debug build on Windows:

```powershell
C:\Users\Dmytro\AppData\Local\Programs\Python\Python313\python.exe -m SCons platform=windows target=template_debug -j1
```

Release build example:

```powershell
C:\Users\Dmytro\AppData\Local\Programs\Python\Python313\python.exe -m SCons platform=windows target=template_release -j1
```

After building other targets, update `godotdetour.gdextension` to include the matching libraries.

## 20. Practical advice

- Start from the demo values before tuning mesh generation parameters.
- Keep your source navmesh geometry simple and clean.
- Define query filters before spawning agents.
- If you see visual stepping, that is usually a presentation issue, not a pathfinding failure.
- If you hit a native crash while integrating, switch temporarily to manual `tick()` mode to debug on the main thread.

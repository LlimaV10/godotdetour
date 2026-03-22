## Documentation stub for the native `DetourNavigation` GDExtension class.
## This file is intentionally kept in a `.gdignore` directory so it does not
## conflict with the runtime class registration.
class_name DetourNavigation
extends RefCounted

## Registered signal for per-tick timing information.
## Note: the current Godot 4 port binds this signal but does not emit it.
signal navigation_tick_done(executionTimeSeconds: float)


## Builds the navigation system from a `MeshInstance3D` and starts the internal
## threaded update loop in the current implementation.
## If you create agents with external movement mode, the thread still computes
## steering, but your scene is expected to move those actors itself.
func initialize(input_mesh_instance: Variant, parameters: DetourNavigationParameters) -> bool:
	return false


## Manually advances the navigation simulation.
## This is available as a safer debugging/integration path if you choose to
## drive the addon from the main thread instead of using the internal thread.
func tick(delta_seconds: float = -1.0) -> void:
	pass


## Rebuilds navigation tiles affected by changed convex areas or off-mesh
## connections.
func rebuildChangedTiles() -> void:
	pass


## Marks a convex navigation area and returns its internal ID.
## `vertices` should be the corner points of the area on the source mesh.
## `height` extrudes the area upward.
## `area_type` is the Detour/Recast area identifier to assign.
# - 0: POLY_AREA_GROUND
# - 1: POLY_AREA_WATER
# - 2: POLY_AREA_ROAD
# - 3: POLY_AREA_DOOR
# - 4: POLY_AREA_GRASS
# - 5: POLY_AREA_JUMP
func markConvexArea(vertices: Array, height: float, area_type: int) -> int:
	return -1


## Adds an off-mesh connection and returns its internal ID.
func addOffMeshConnection(
	from: Vector3, to: Vector3, bidirectional: bool, radius: float, area_type: int
) -> int:
	return -1


## Removes a previously created off-mesh connection by ID.
func removeOffMeshConnection(id: int) -> void:
	pass


## Defines a named query filter used by crowd agents.
## `weights` maps area indices to movement costs.
func setQueryFilter(index: int, name: String, weights: Dictionary) -> bool:
	return false


## Creates a new Detour crowd agent from the supplied parameters.
## The agent can run either in simulated-position mode or external-position mode
## depending on `parameters.movementMode`.
func addAgent(parameters: DetourCrowdAgentParameters) -> DetourCrowdAgent:
	return null


## Removes an agent from the simulation.
func removeAgent(agent: DetourCrowdAgent) -> void:
	pass


## Adds a box obstacle. `dimensions` are the full size, not half extents.
func addBoxObstacle(position: Vector3, dimensions: Vector3, rotation_rad: float) -> DetourObstacle:
	return null


## Adds a cylindrical obstacle.
func addCylinderObstacle(position: Vector3, radius: float, height: float) -> DetourObstacle:
	return null


## Creates a debug `MeshInstance3D` visualizing the generated navmesh.
func createDebugMesh(index: int, draw_cache_bounds: bool) -> MeshInstance3D:
	return null


## Saves the native navigation state to disk.
func save(path: String, compressed: bool) -> bool:
	return false


## Loads a previously saved navigation state from disk.
func load(path: String, compressed: bool) -> bool:
	return false


## Stops updates and frees navigation data, agents, and obstacles.
func clear() -> void:
	pass


## Returns all currently tracked `DetourCrowdAgent` objects.
func getAgents() -> Array:
	return []


## Returns all active `DetourObstacle` objects.
func getObstacles() -> Array:
	return []


## Returns the IDs of all currently marked convex areas.
func getMarkedAreaIDs() -> Array:
	return []


## Returns `true` once `initialize()` or `load()` has completed successfully.
func isInitialized() -> bool:
	return false

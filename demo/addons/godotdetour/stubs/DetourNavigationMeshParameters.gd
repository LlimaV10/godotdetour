## Documentation stub for the native `DetourNavigationMeshParameters` class.
class_name DetourNavigationMeshParameters
extends RefCounted

## Horizontal and vertical voxel size used during Recast rasterization.
var cellSize: Vector2

## Maximum number of agents supported by this navmesh/crowd profile.
var maxNumAgents: int = 256

## Maximum walkable slope angle in degrees.
var maxAgentSlope: float = 0.0

## Required standing clearance for agents using this profile.
var maxAgentHeight: float = 0.0

## Maximum ledge or step height an agent can climb.
var maxAgentClimb: float = 0.0

## Maximum supported agent radius for this navmesh profile.
var maxAgentRadius: float = 0.0

## Maximum contour edge length before extra simplification points are inserted.
var maxEdgeLength: float = 0.0

## Allowed contour simplification error. Larger values simplify more.
var maxSimplificationError: float = 0.0

## Minimum isolated region area retained in the final navmesh.
var minNumCellsPerIsland: int = 0

## Region merge threshold used during Recast region building.
var minCellSpanCount: int = 0

## Maximum number of vertices per polygon in the generated navmesh.
var maxVertsPerPoly: int = 0

## Tile resolution, in cells, used by the tiled navmesh and tile cache.
var tileSize: int = 0

## Maximum number of compressed tile layers stored per tile.
var layersPerTile: int = 0

## Detail mesh sample spacing multiplier.
var detailSampleDistance: float = 0.0

## Maximum allowed height error for the detail mesh.
var detailSampleMaxError: float = 0.0

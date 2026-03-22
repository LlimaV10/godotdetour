## Documentation stub for the native `DetourCrowdAgentParameters` class.
class_name DetourCrowdAgentParameters
extends RefCounted

## Native addon owns and advances the agent position.
const MOVEMENT_MODE_SIMULATED := 0

## Native addon computes steering and desired velocity, but Godot is expected
## to move the actor and report the final position back with `syncPosition()`.
const MOVEMENT_MODE_EXTERNAL := 1

## Initial world-space position of the agent.
var position: Vector3

## Agent radius used for crowd collision and navmesh fit selection.
var radius: float = 0.0

## Agent standing height.
var height: float = 0.0

## Maximum acceleration used by the Detour crowd update.
var maxAcceleration: float = 0.0

## Maximum movement speed.
var maxSpeed: float = 0.0

## Name of the query filter defined by `DetourNavigation.setQueryFilter()`.
var filterName: String = "default"

## If true, the crowd solver anticipates upcoming turns.
var anticipateTurns: bool = true

## If true, visibility optimization is enabled.
var optimizeVisibility: bool = true

## If true, topology optimization is enabled.
var optimizeTopology: bool = true

## If true, obstacle avoidance is enabled.
var avoidObstacles: bool = true

## If true, the agent separates from nearby agents.
var avoidOtherAgents: bool = true

## Index of the Detour obstacle avoidance profile to use.
var obstacleAvoidance: int = 0

## Strength of crowd separation from nearby agents.
var separationWeight: float = 0.0

## Agent movement mode.
## Use `MOVEMENT_MODE_SIMULATED` to let the addon own position updates.
## Use `MOVEMENT_MODE_EXTERNAL` to let Godot own movement while the addon only
## computes steering and desired velocity.
var movementMode: int = MOVEMENT_MODE_SIMULATED

## Documentation stub for the native `DetourCrowdAgent` class.
class_name DetourCrowdAgent
extends RefCounted

## Reserved arrival signal. Registered in native code but not emitted by the
## current Godot 4 runtime path.
signal arrived_at_target(node: Object)

## Reserved stalled-progress signal. Registered in native code but not emitted
## by the current Godot 4 runtime path.
signal no_progress(node: Object, distanceLeft: float)

## Reserved no-movement signal. Registered in native code but not emitted by
## the current Godot 4 runtime path.
signal no_movement(node: Object)

## Current simulated world-space position.
var position: Vector3:
	get:
		return Vector3.ZERO

## Current simulated velocity.
## In simulated mode this is the velocity of the native agent.
## In external mode this is also the steering velocity produced by the native
## simulation.
var velocity: Vector3:
	get:
		return Vector3.ZERO

## Desired movement velocity. In external-position mode this is the value you
## should typically apply from Godot-side movement code.
var desiredVelocity: Vector3:
	get:
		return Vector3.ZERO

## Current requested move target.
var target: Vector3:
	get:
		return Vector3.ZERO

## Whether the agent is currently considered moving.
var isMoving: bool:
	get:
		return false

## Sets a new target position for the agent.
func moveTowards(position: Vector3) -> void:
	pass

## Updates the authoritative world position for agents running in external
## movement mode.
## Call this after your Godot-side movement step, for example after
## `move_and_slide()` in `_physics_process()`.
func syncPosition(position: Vector3) -> void:
	pass

## Clears the current move target and makes the agent idle.
func stop() -> void:
	pass

## Returns a dictionary with predicted `position` and `direction` values for
## smoothing visible motion between simulation updates.
func getPredictedMovement(current_pos: Vector3, current_dir: Vector3, position_ticks_timestamp: int, max_turning_rad: float) -> Dictionary:
	return {}

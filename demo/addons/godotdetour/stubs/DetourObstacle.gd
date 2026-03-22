## Documentation stub for the native `DetourObstacle` class.
class_name DetourObstacle
extends RefCounted

## Current obstacle position.
var position: Vector3

## Obstacle size. For boxes, this is the full size. For cylinders, `x` stores
## radius and `y` stores height.
var dimensions: Vector3

## Moves the obstacle by removing and recreating it in the tile cache.
func move(position: Vector3) -> void:
	pass

## Removes the obstacle from all native tile caches.
func destroy() -> void:
	pass

## Returns `true` if the obstacle has been destroyed already.
func is_destroyed() -> bool:
	return false

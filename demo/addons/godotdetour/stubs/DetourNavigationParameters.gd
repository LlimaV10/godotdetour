## Documentation stub for the native `DetourNavigationParameters` class.
class_name DetourNavigationParameters
extends RefCounted

## Array of `DetourNavigationMeshParameters` profiles used to build one or more
## navmeshes.
var navMeshParameters: Array = []

## Update frequency used by the threaded navigation loop and as the default step
## source for `DetourNavigation.tick()`.
var ticksPerSecond: int = 60

## Maximum number of dynamic obstacles supported by the tile cache.
var maxObstacles: int = 256

## Default navigation area type assigned before explicit area marking.
var defaultAreaType: int = 0

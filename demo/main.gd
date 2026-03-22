extends Node3D

const AGENT_COUNT := 3

@onready var info_label: RichTextLabel = $CanvasLayer/Control/Info
@onready var status_label: RichTextLabel = $CanvasLayer/Control/Status
@onready var ground_mesh_instance: MeshInstance3D = $Ground
@onready var start_marker: Marker3D = $Start
@onready var target_a_marker: Marker3D = $TargetA
@onready var target_b_marker: Marker3D = $TargetB

var navigation: DetourNavigation
var debug_mesh_instance: MeshInstance3D
var agent_entries: Array[Dictionary] = []
var next_target_is_a := false


func _ready() -> void:
	navigation = DetourNavigation.new()
	# navigation.tick()
	_initialize_navigation()
	_spawn_agents()
	_draw_debug_mesh()
	info_label.text = "[b]GodotDetour Godot 4 Demo[/b]\nSpace retargets all agents."
	status_label.text = "Navigation initialized."


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		_retarget_agents()


func _process(delta: float) -> void:
	var moving_count := 0
	for entry in agent_entries:
		var actor: Node3D = entry["node"]
		var detour_agent: DetourCrowdAgent = entry["agent"]

		# 1st mode: detour controls position
		actor.global_position = detour_agent.position

		# 2nd mode: godot controls position
		# actor.global_position += detour_agent.desiredVelocity.normalized() * delta * 5.0
		# detour_agent.syncPosition(actor.global_position)

		if detour_agent.isMoving:
			moving_count += 1
		# var velocity := detour_agent.velocity
		# velocity.y = 0.0
		# if velocity.length_squared() > 0.001:
		# 	actor.look_at(actor.global_position + velocity.normalized(), Vector3.UP)

	if navigation != null and navigation.isInitialized():
		status_label.text = (
			"Agents moving: %d/%d. Press Space to retarget." % [moving_count, agent_entries.size()]
		)


func _exit_tree() -> void:
	if navigation != null and navigation.isInitialized():
		navigation.clear()


func _initialize_navigation() -> void:
	var nav_params := DetourNavigationParameters.new()
	nav_params.ticksPerSecond = 15
	nav_params.maxObstacles = 64

	var nav_mesh_params := DetourNavigationMeshParameters.new()
	nav_mesh_params.cellSize = Vector2(0.25, 0.2)
	nav_mesh_params.maxNumAgents = 32
	nav_mesh_params.maxAgentSlope = 45.0
	nav_mesh_params.maxAgentHeight = 2.2
	nav_mesh_params.maxAgentClimb = 0.6
	nav_mesh_params.maxAgentRadius = 0.5
	nav_mesh_params.maxEdgeLength = 12.0
	nav_mesh_params.maxSimplificationError = 1.3
	nav_mesh_params.minNumCellsPerIsland = 8
	nav_mesh_params.minCellSpanCount = 20
	nav_mesh_params.maxVertsPerPoly = 6
	nav_mesh_params.tileSize = 48
	nav_mesh_params.layersPerTile = 4
	nav_mesh_params.detailSampleDistance = 6.0
	nav_mesh_params.detailSampleMaxError = 1.0
	nav_params.navMeshParameters.append(nav_mesh_params)

	var weights := {
		0: 1.0,
		1: 1.0,
		2: 999999.0,
		3: 2.0,
		4: 3.0,
		5: 3.0,
	}

	var ok := navigation.initialize(ground_mesh_instance, nav_params)
	if not ok:
		status_label.text = "Initialization failed."
		push_error("DetourNavigation failed to initialize.")
		return

	navigation.setQueryFilter(0, "default", weights)

	var area_vertices: Array = [
		Vector3(-3.0, -0.25, -3.0),
		Vector3(3.0, -0.25, -3.0),
		Vector3(3.0, -0.25, 3.0),
		Vector3(-3.0, -0.25, 3.0),
	]
	navigation.markConvexArea(area_vertices, 0.8, 4)
	navigation.rebuildChangedTiles()


func _spawn_agents() -> void:
	for i in range(AGENT_COUNT):
		var params := DetourCrowdAgentParameters.new()
		params.position = start_marker.global_position + Vector3(float(i) * 1.6 - 1.6, 0.0, 0.0)
		params.radius = 0.35
		params.height = 1.7
		params.maxAcceleration = 8.0
		params.maxSpeed = 3.0
		params.filterName = "default"
		params.anticipateTurns = true
		params.optimizeVisibility = true
		params.optimizeTopology = true
		params.avoidObstacles = true
		params.avoidOtherAgents = true
		params.obstacleAvoidance = 1
		params.separationWeight = 1.5
		params.movementMode = 0

		var detour_agent := navigation.addAgent(params)
		if detour_agent == null:
			push_error("Failed to create demo agent %d" % i)
			continue

		var actor := _make_agent_visual(i)
		actor.position = params.position
		add_child(actor)

		(
			agent_entries
			. append(
				{
					"node": actor,
					"agent": detour_agent,
				}
			)
		)

	_retarget_agents()


func _make_agent_visual(index: int) -> Node3D:
	var root := Node3D.new()
	root.name = "Agent%d" % index

	var body := MeshInstance3D.new()
	var capsule := CapsuleMesh.new()
	capsule.radius = 0.35
	capsule.height = 1.7
	body.mesh = capsule
	body.position = Vector3(0.0, 0.85, 0.0)

	var material := StandardMaterial3D.new()
	var colors := [
		Color(0.12, 0.45, 0.88),
		Color(0.92, 0.38, 0.16),
		Color(0.18, 0.70, 0.32),
	]
	material.albedo_color = colors[index % colors.size()]
	body.material_override = material

	root.add_child(body)
	return root


func _retarget_agents() -> void:
	next_target_is_a = not next_target_is_a
	var target := (
		target_a_marker.global_position if next_target_is_a else target_b_marker.global_position
	)
	for entry in agent_entries:
		var detour_agent: DetourCrowdAgent = entry["agent"]
		detour_agent.moveTowards(target)
	status_label.text = "Retargeted to %s." % ("Target A" if next_target_is_a else "Target B")


func _draw_debug_mesh() -> void:
	if not navigation.isInitialized():
		return

	if debug_mesh_instance != null:
		debug_mesh_instance.queue_free()

	debug_mesh_instance = navigation.createDebugMesh(0, false)
	if debug_mesh_instance == null:
		status_label.text = "Debug mesh creation failed."
		return

	debug_mesh_instance.position.y += 0.05
	add_child(debug_mesh_instance)

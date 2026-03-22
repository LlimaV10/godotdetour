extends Camera3D

@export_range(0.01, 2.0, 0.01) var sensitivity: float = 0.12
@export_range(0.1, 20.0, 0.1) var speed: float = 8.0
@export_range(0.0, 89.0, 0.1) var pitch_limit: float = 80.0

var _mouse_delta := Vector2.ZERO
var _pitch := 0.0

func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		_mouse_delta += event.relative
	elif event.is_action_pressed("ui_cancel"):
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	elif event is InputEventMouseButton and event.pressed and Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _process(delta: float) -> void:
	if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_rotate_camera(delta)
	_move_camera(delta)

func _rotate_camera(_delta: float) -> void:
	if _mouse_delta == Vector2.ZERO:
		return

	rotate_y(deg_to_rad(-_mouse_delta.x * sensitivity))
	_pitch = clamp(_pitch - _mouse_delta.y * sensitivity, -pitch_limit, pitch_limit)
	rotation_degrees.x = _pitch
	_mouse_delta = Vector2.ZERO

func _move_camera(delta: float) -> void:
	var input := Vector3.ZERO
	if Input.is_action_pressed("move_forward"):
		input.z -= 1.0
	if Input.is_action_pressed("move_back"):
		input.z += 1.0
	if Input.is_action_pressed("strafe_left"):
		input.x -= 1.0
	if Input.is_action_pressed("strafe_right"):
		input.x += 1.0
	if Input.is_action_pressed("up"):
		input.y += 1.0
	if Input.is_action_pressed("down"):
		input.y -= 1.0

	if input == Vector3.ZERO:
		return

	global_position += global_basis * input.normalized() * speed * delta

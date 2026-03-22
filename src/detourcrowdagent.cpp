#include "detourcrowdagent.h"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>

#include <DetourCrowd.h>
#include <DetourNavMeshQuery.h>

#include "util/detourinputgeometry.h"

using namespace godot;

#define AGENT_SAVE_VERSION 2

void DetourCrowdAgentParameters::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_position"), &DetourCrowdAgentParameters::get_position);
    ClassDB::bind_method(D_METHOD("set_position", "position"), &DetourCrowdAgentParameters::set_position);
    ClassDB::bind_method(D_METHOD("get_radius"), &DetourCrowdAgentParameters::get_radius);
    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &DetourCrowdAgentParameters::set_radius);
    ClassDB::bind_method(D_METHOD("get_height"), &DetourCrowdAgentParameters::get_height);
    ClassDB::bind_method(D_METHOD("set_height", "height"), &DetourCrowdAgentParameters::set_height);
    ClassDB::bind_method(D_METHOD("get_max_acceleration"), &DetourCrowdAgentParameters::get_max_acceleration);
    ClassDB::bind_method(D_METHOD("set_max_acceleration", "value"), &DetourCrowdAgentParameters::set_max_acceleration);
    ClassDB::bind_method(D_METHOD("get_max_speed"), &DetourCrowdAgentParameters::get_max_speed);
    ClassDB::bind_method(D_METHOD("set_max_speed", "value"), &DetourCrowdAgentParameters::set_max_speed);
    ClassDB::bind_method(D_METHOD("get_filter_name"), &DetourCrowdAgentParameters::get_filter_name);
    ClassDB::bind_method(D_METHOD("set_filter_name", "value"), &DetourCrowdAgentParameters::set_filter_name);
    ClassDB::bind_method(D_METHOD("get_anticipate_turns"), &DetourCrowdAgentParameters::get_anticipate_turns);
    ClassDB::bind_method(D_METHOD("set_anticipate_turns", "value"), &DetourCrowdAgentParameters::set_anticipate_turns);
    ClassDB::bind_method(D_METHOD("get_optimize_visibility"), &DetourCrowdAgentParameters::get_optimize_visibility);
    ClassDB::bind_method(D_METHOD("set_optimize_visibility", "value"), &DetourCrowdAgentParameters::set_optimize_visibility);
    ClassDB::bind_method(D_METHOD("get_optimize_topology"), &DetourCrowdAgentParameters::get_optimize_topology);
    ClassDB::bind_method(D_METHOD("set_optimize_topology", "value"), &DetourCrowdAgentParameters::set_optimize_topology);
    ClassDB::bind_method(D_METHOD("get_avoid_obstacles"), &DetourCrowdAgentParameters::get_avoid_obstacles);
    ClassDB::bind_method(D_METHOD("set_avoid_obstacles", "value"), &DetourCrowdAgentParameters::set_avoid_obstacles);
    ClassDB::bind_method(D_METHOD("get_avoid_other_agents"), &DetourCrowdAgentParameters::get_avoid_other_agents);
    ClassDB::bind_method(D_METHOD("set_avoid_other_agents", "value"), &DetourCrowdAgentParameters::set_avoid_other_agents);
    ClassDB::bind_method(D_METHOD("get_obstacle_avoidance"), &DetourCrowdAgentParameters::get_obstacle_avoidance);
    ClassDB::bind_method(D_METHOD("set_obstacle_avoidance", "value"), &DetourCrowdAgentParameters::set_obstacle_avoidance);
    ClassDB::bind_method(D_METHOD("get_separation_weight"), &DetourCrowdAgentParameters::get_separation_weight);
    ClassDB::bind_method(D_METHOD("set_separation_weight", "value"), &DetourCrowdAgentParameters::set_separation_weight);
    ClassDB::bind_method(D_METHOD("get_movement_mode"), &DetourCrowdAgentParameters::get_movement_mode);
    ClassDB::bind_method(D_METHOD("set_movement_mode", "value"), &DetourCrowdAgentParameters::set_movement_mode);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height"), "set_height", "get_height");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "maxAcceleration"), "set_max_acceleration", "get_max_acceleration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "maxSpeed"), "set_max_speed", "get_max_speed");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "filterName"), "set_filter_name", "get_filter_name");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anticipateTurns"), "set_anticipate_turns", "get_anticipate_turns");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "optimizeVisibility"), "set_optimize_visibility", "get_optimize_visibility");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "optimizeTopology"), "set_optimize_topology", "get_optimize_topology");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidObstacles"), "set_avoid_obstacles", "get_avoid_obstacles");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidOtherAgents"), "set_avoid_other_agents", "get_avoid_other_agents");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "obstacleAvoidance"), "set_obstacle_avoidance", "get_obstacle_avoidance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "separationWeight"), "set_separation_weight", "get_separation_weight");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "movementMode"), "set_movement_mode", "get_movement_mode");
}

void DetourCrowdAgent::_bind_methods() {
    ClassDB::bind_method(D_METHOD("moveTowards", "position"), &DetourCrowdAgent::move_towards);
    ClassDB::bind_method(D_METHOD("stop"), &DetourCrowdAgent::stop);
    ClassDB::bind_method(D_METHOD("getPredictedMovement", "current_pos", "current_dir", "position_ticks_timestamp", "max_turning_rad"), &DetourCrowdAgent::get_predicted_movement);
    ClassDB::bind_method(D_METHOD("get_position"), &DetourCrowdAgent::get_position);
    ClassDB::bind_method(D_METHOD("get_velocity"), &DetourCrowdAgent::get_velocity);
    ClassDB::bind_method(D_METHOD("get_desired_velocity"), &DetourCrowdAgent::get_desired_velocity);
    ClassDB::bind_method(D_METHOD("get_target_position"), &DetourCrowdAgent::get_target_position);
    ClassDB::bind_method(D_METHOD("is_moving"), &DetourCrowdAgent::is_moving);
    ClassDB::bind_method(D_METHOD("syncPosition", "position"), &DetourCrowdAgent::sync_position);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "", "get_position");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "velocity"), "", "get_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "desiredVelocity"), "", "get_desired_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "target"), "", "get_target_position");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "isMoving"), "", "is_moving");

    ADD_SIGNAL(MethodInfo("arrived_at_target", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("no_progress", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::FLOAT, "distanceLeft")));
    ADD_SIGNAL(MethodInfo("no_movement", PropertyInfo(Variant::OBJECT, "node")));
}

DetourCrowdAgent::DetourCrowdAgent()
        : _agent(nullptr),
          _crowd(nullptr),
          _agent_index(0),
          _crowd_index(0),
          _query(nullptr),
          _filter(nullptr),
          _filter_index(0),
          _input_geom(nullptr),
          _external_position(),
          _has_new_target(false),
          _has_pending_external_position(false),
          _state(AGENT_STATE_INVALID),
          _movement_mode(AGENT_MOVEMENT_MODE_SIMULATED),
          _is_moving(false),
          _last_distance_to_target(0.0f),
          _distance_total(0.0f),
          _distance_time(0.0f),
          _movement_time(0.0f),
          _movement_over_time(0.0f) {
    last_update_time = std::chrono::system_clock::now();
}

DetourCrowdAgent::~DetourCrowdAgent() {}

bool DetourCrowdAgent::is_moving() const {
    std::lock_guard<std::mutex> lock(_state_mutex);
    return _is_moving;
}

Vector3 DetourCrowdAgent::get_target_position() const {
    std::lock_guard<std::mutex> lock(_state_mutex);
    return _target_position;
}

Vector3 DetourCrowdAgent::get_position() const {
    std::lock_guard<std::mutex> lock(_state_mutex);
    return _position;
}

Vector3 DetourCrowdAgent::get_velocity() const {
    std::lock_guard<std::mutex> lock(_state_mutex);
    return _velocity;
}

Vector3 DetourCrowdAgent::get_desired_velocity() const {
    return get_velocity();
}

bool DetourCrowdAgent::save(const Ref<FileAccess> &target_file) {
    if (!_agent) {
        ERR_PRINT("AgentSave: No detour agent present!");
        return false;
    }

    target_file->store_16(AGENT_SAVE_VERSION);
    target_file->store_32(_agent_index);
    target_file->store_32(_crowd_index);
    target_file->store_32(_filter_index);
    target_file->store_32(_movement_mode);
    target_file->store_var(_position);
    target_file->store_var(_velocity);
    target_file->store_var(_target_position);
    target_file->store_8(_has_new_target.load());
    target_file->store_8(_is_moving);
    target_file->store_16(_state);

    target_file->store_float(_agent->params.radius);
    target_file->store_float(_agent->params.height);
    target_file->store_float(_agent->params.maxAcceleration);
    target_file->store_float(_agent->params.maxSpeed);
    target_file->store_32(_agent->params.updateFlags);
    target_file->store_8(_agent->params.obstacleAvoidanceType);
    target_file->store_float(_agent->params.separationWeight);

    return true;
}

bool DetourCrowdAgent::load(const Ref<FileAccess> &source_file) {
    int version = source_file->get_16();

    if (version != 1 && version != AGENT_SAVE_VERSION) {
        ERR_PRINT(String("Unable to load agent. Unknown save version: {0}").format(Array::make(version)));
        return false;
    }

    _agent_index = source_file->get_32();
    _crowd_index = source_file->get_32();
    _filter_index = source_file->get_32();
    _movement_mode = version >= 2 ? source_file->get_32() : AGENT_MOVEMENT_MODE_SIMULATED;
    _position = source_file->get_var(true);
    _velocity = source_file->get_var(true);
    _target_position = source_file->get_var(true);
    _external_position = _position;
    _has_pending_external_position = false;
    _has_new_target = source_file->get_8();
    _is_moving = source_file->get_8();
    _state = static_cast<DetourCrowdAgentState>(source_file->get_16());
    return true;
}

bool DetourCrowdAgent::load_parameter_values(const Ref<DetourCrowdAgentParameters> &params, const Ref<FileAccess> &source_file) {
    params->radius = source_file->get_float();
    params->height = source_file->get_float();
    params->maxAcceleration = source_file->get_float();
    params->maxSpeed = source_file->get_float();
    params->position = _position;
    int update_flags = source_file->get_32();
    params->anticipateTurns = update_flags & DT_CROWD_ANTICIPATE_TURNS;
    params->optimizeVisibility = update_flags & DT_CROWD_OPTIMIZE_VIS;
    params->optimizeTopology = update_flags & DT_CROWD_OPTIMIZE_TOPO;
    params->avoidObstacles = update_flags & DT_CROWD_OBSTACLE_AVOIDANCE;
    params->avoidOtherAgents = update_flags & DT_CROWD_SEPARATION;
    params->obstacleAvoidance = source_file->get_8();
    params->separationWeight = source_file->get_float();
    params->movementMode = _movement_mode;
    return true;
}

void DetourCrowdAgent::set_movement_mode(int movement_mode) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _movement_mode = movement_mode;
}

void DetourCrowdAgent::set_main_agent(dtCrowdAgent *crowd_agent, dtCrowd *crowd, int index, dtNavMeshQuery *query, DetourInputGeometry *geom, int crowd_index) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _agent = crowd_agent;
    _crowd = crowd;
    _agent_index = index;
    _crowd_index = crowd_index;
    _query = query;
    if (_crowd != nullptr) {
        _filter = _crowd->getEditableFilter(_filter_index);
    }
    _input_geom = geom;
    _state = AGENT_STATE_IDLE;
    _distance_total = 0.0f;
    _last_distance_to_target = 0.0f;
    _distance_time = 0.0f;
    _movement_over_time = 0.0f;
    _movement_time = 0.0f;
    _last_position = Vector3(_agent->npos[0], _agent->npos[1], _agent->npos[2]);
    _position = _last_position;
    _external_position = _last_position;
}

void DetourCrowdAgent::set_filter(int filter_index) {
    _filter_index = filter_index;
    if (_crowd != nullptr) {
        _filter = _crowd->getEditableFilter(filter_index);
    }
}

void DetourCrowdAgent::add_shadow_agent(dtCrowdAgent *crowd_agent) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _shadows.push_back(crowd_agent);
}

void DetourCrowdAgent::move_towards(Vector3 position) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _target_position = position;
    _has_new_target = true;
    _distance_total = 0.0f;
    _last_distance_to_target = 0.0f;
    _movement_time = 0.0f;
    _movement_over_time = 0.0f;
    _last_position = _position;
}

void DetourCrowdAgent::sync_position(Vector3 position) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _external_position = position;
    _position = position;
    _has_pending_external_position = true;
}

void DetourCrowdAgent::prepare_for_tick() {
    std::lock_guard<std::mutex> lock(_state_mutex);
    if (_movement_mode != AGENT_MOVEMENT_MODE_EXTERNAL || _agent == nullptr) {
        return;
    }

    if (_has_pending_external_position) {
        _has_pending_external_position = false;
    }

    _agent->npos[0] = _external_position.x;
    _agent->npos[1] = _external_position.y;
    _agent->npos[2] = _external_position.z;
    for (dtCrowdAgent *shadow : _shadows) {
        shadow->npos[0] = _external_position.x;
        shadow->npos[1] = _external_position.y;
        shadow->npos[2] = _external_position.z;
    }
}

void DetourCrowdAgent::apply_new_target() {
    if (!_has_new_target) {
        return;
    }
    std::lock_guard<std::mutex> lock(_state_mutex);
    if (_crowd == nullptr || _query == nullptr || _filter == nullptr) {
        _has_new_target = true;
        ERR_PRINT("applyNewTarget: Agent query/filter not initialized yet.");
        return;
    }
    _has_new_target = false;

    dtPolyRef target_ref;
    float final_target_pos[3] = {};
    const float *half_extents = _crowd->getQueryExtents();
    float pos[3] = { _target_position.x, _target_position.y, _target_position.z };
    float extents[3] = { half_extents[0], half_extents[1], half_extents[2] };

    dtStatus status = _query->findNearestPoly(pos, extents, _filter, &target_ref, final_target_pos);
    if (dtStatusFailed(status)) {
        _has_new_target = true;
        ERR_PRINT(String("applyNewTarget: findPoly failed: {0}").format(Array::make(status)));
        return;
    }

    if (!_crowd->requestMoveTarget(_agent_index, target_ref, final_target_pos)) {
        ERR_PRINT("Unable to request detour move target.");
    }
    _state = AGENT_STATE_GOING_TO_TARGET;
}

void DetourCrowdAgent::stop() {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _crowd->resetMoveTarget(_agent_index);
    _has_new_target = false;
    _is_moving = false;
    _state = AGENT_STATE_IDLE;
    _distance_total = 0.0f;
    _last_distance_to_target = 0.0f;
    _distance_time = 0.0f;
    _movement_time = 0.0f;
    _movement_over_time = 0.0f;
}

Dictionary DetourCrowdAgent::get_predicted_movement(Vector3 current_pos, Vector3 current_dir, int64_t position_ticks_timestamp, float max_turning_rad) {
    Dictionary result;

    auto time_since_update = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_update_time).count();
    float seconds_passed = time_since_update / 1000.0f;

    Vector3 vel_to_use = _velocity.length() <= 0.01f ? current_dir : _velocity;
    Vector3 agent_target_pos = _position + seconds_passed * vel_to_use;
    float distance = current_pos.distance_to(agent_target_pos);
    if (distance < 0.01f) {
        result["position"] = current_pos;
        vel_to_use.y = 0.0f;
        vel_to_use = vel_to_use.normalized();
        result["direction"] = vel_to_use;
        return result;
    }

    float seconds_since_timestamp = (Time::get_singleton()->get_ticks_msec() - position_ticks_timestamp) / 1000.0f;
    Vector3 direction = (agent_target_pos - current_pos).normalized();
    float length_to_use = vel_to_use.length();
    Vector3 movement = seconds_since_timestamp * (direction * length_to_use);
    if (movement.length() > distance) {
        movement = movement.normalized() * distance;
    }

    direction.y = 0.0f;
    direction = direction.normalized();
    float turning_rad = current_dir.angle_to(direction);
    turning_rad = current_dir.cross(direction).y > 0.0f ? turning_rad : -turning_rad;
    if (Math::abs(turning_rad) > max_turning_rad) {
        turning_rad = turning_rad < 0.0f ? -max_turning_rad : max_turning_rad;
    }

    result["position"] = current_pos + movement;
    result["direction"] = current_dir.rotated(Vector3(0.0f, 1.0f, 0.0f), turning_rad);
    return result;
}

void DetourCrowdAgent::update(float seconds_since_last_tick) {
    std::lock_guard<std::mutex> lock(_state_mutex);
    _velocity = Vector3(_agent->vel[0], _agent->vel[1], _agent->vel[2]);

    if (_movement_mode == AGENT_MOVEMENT_MODE_EXTERNAL) {
        _position = _external_position;

        if (_state == AGENT_STATE_GOING_TO_TARGET) {
            const float distance_to_target = _target_position.distance_to(_external_position);
            _is_moving = _velocity.length_squared() > 0.001f;
            if (distance_to_target < 0.1f) {
                _is_moving = false;
                _crowd->resetMoveTarget(_agent_index);
                _state = AGENT_STATE_IDLE;
            }
            _last_distance_to_target = distance_to_target;
        } else {
            _is_moving = false;
        }

        _agent->npos[0] = _external_position.x;
        _agent->npos[1] = _external_position.y;
        _agent->npos[2] = _external_position.z;
        for (dtCrowdAgent *shadow : _shadows) {
            shadow->npos[0] = _external_position.x;
            shadow->npos[1] = _external_position.y;
            shadow->npos[2] = _external_position.z;
        }
        _last_position = _external_position;
        return;
    }

    for (dtCrowdAgent *shadow : _shadows) {
        shadow->npos[0] = _agent->npos[0];
        shadow->npos[1] = _agent->npos[1];
        shadow->npos[2] = _agent->npos[2];
    }

    _position = Vector3(_agent->npos[0], _agent->npos[1], _agent->npos[2]);

    switch (_state) {
        case AGENT_STATE_GOING_TO_TARGET: {
            last_update_time = std::chrono::system_clock::now();
            float distance_to_target = _target_position.distance_to(_position);
            _distance_time += seconds_since_last_tick;
            _distance_total += Math::abs(_last_distance_to_target - distance_to_target);
            _movement_over_time += _position.distance_squared_to(_last_position);
            _is_moving = _movement_over_time > 0.001f;

            if (_is_moving && _velocity.length_squared() <= 0.001f) {
                _velocity = (_position - _last_position).normalized() / seconds_since_last_tick;
            }

            _last_position = _position;
            _movement_time += seconds_since_last_tick;

            if (_movement_time >= 1.0f) {
                _movement_time -= 1.0f;
                if (_movement_over_time < (_agent->params.maxSpeed * 0.01f)) {
                    // Signals are disabled here because update() runs on the navigation worker thread.
                }
                _movement_over_time = 0.0f;
            }

            if (_distance_time >= 5.0f) {
                _distance_time -= 5.0f;
                if (_distance_total < (_agent->params.maxSpeed * 0.03f)) {
                    // Signals are disabled here because update() runs on the navigation worker thread.
                }
                _distance_total = 0.0f;
            }

            if (distance_to_target < 0.1f) {
                _is_moving = false;
                _crowd->resetMoveTarget(_agent_index);
                _state = AGENT_STATE_IDLE;
                _distance_total = 0.0f;
                _last_distance_to_target = 0.0f;
                _distance_time = 0.0f;
                _movement_time = 0.0f;
                _movement_over_time = 0.0f;
            }
            _last_distance_to_target = distance_to_target;
        } break;
        default:
            break;
    }
}

void DetourCrowdAgent::destroy() {
    std::lock_guard<std::mutex> lock(_state_mutex);
    if (!_agent) {
        return;
    }

    _agent->active = false;
    for (dtCrowdAgent *shadow : _shadows) {
        shadow->active = false;
    }
    _shadows.clear();
    _agent = nullptr;
    _is_moving = false;
    _distance_total = 0.0f;
    _last_distance_to_target = 0.0f;
    _distance_time = 0.0f;
    _movement_time = 0.0f;
    _movement_over_time = 0.0f;
}

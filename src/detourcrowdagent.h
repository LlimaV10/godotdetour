#ifndef DETOURCROWDAGENT_H
#define DETOURCROWDAGENT_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>

class dtCrowd;
class dtCrowdAgent;
class dtNavMeshQuery;
class dtQueryFilter;
class DetourInputGeometry;

namespace godot {

class DetourNavigationMesh;

enum DetourCrowdAgentMovementMode {
    AGENT_MOVEMENT_MODE_SIMULATED = 0,
    AGENT_MOVEMENT_MODE_EXTERNAL = 1,
};

class DetourCrowdAgentParameters : public RefCounted {
    GDCLASS(DetourCrowdAgentParameters, RefCounted)

public:
    static void _bind_methods();

    void set_position(const Vector3 &p_value);
    Vector3 get_position() const;
    void set_radius(float p_value);
    float get_radius() const;
    void set_height(float p_value);
    float get_height() const;
    void set_max_acceleration(float p_value);
    float get_max_acceleration() const;
    void set_max_speed(float p_value);
    float get_max_speed() const;
    void set_filter_name(const String &p_value);
    String get_filter_name() const;
    void set_anticipate_turns(bool p_value);
    bool get_anticipate_turns() const;
    void set_optimize_visibility(bool p_value);
    bool get_optimize_visibility() const;
    void set_optimize_topology(bool p_value);
    bool get_optimize_topology() const;
    void set_avoid_obstacles(bool p_value);
    bool get_avoid_obstacles() const;
    void set_avoid_other_agents(bool p_value);
    bool get_avoid_other_agents() const;
    void set_obstacle_avoidance(int p_value);
    int get_obstacle_avoidance() const;
    void set_separation_weight(float p_value);
    float get_separation_weight() const;
    void set_movement_mode(int p_value);
    int get_movement_mode() const;

    Vector3 position;
    float radius = 0.0f;
    float height = 0.0f;
    float maxAcceleration = 0.0f;
    float maxSpeed = 0.0f;
    String filterName = "default";
    bool anticipateTurns = true;
    bool optimizeVisibility = true;
    bool optimizeTopology = true;
    bool avoidObstacles = true;
    bool avoidOtherAgents = true;
    int obstacleAvoidance = 0;
    float separationWeight = 0.0f;
    int movementMode = AGENT_MOVEMENT_MODE_SIMULATED;
};

enum DetourCrowdAgentState {
    AGENT_STATE_INVALID = -1,
    AGENT_STATE_IDLE,
    AGENT_STATE_GOING_TO_TARGET,
    NUM_AGENT_STATES
};

class DetourCrowdAgent : public RefCounted {
    GDCLASS(DetourCrowdAgent, RefCounted)

public:
    static void _bind_methods();

    DetourCrowdAgent();
    ~DetourCrowdAgent();

    bool save(const Ref<FileAccess> &target_file);
    bool load(const Ref<FileAccess> &source_file);
    bool load_parameter_values(const Ref<DetourCrowdAgentParameters> &params, const Ref<FileAccess> &source_file);
    void set_movement_mode(int movement_mode);
    void set_main_agent(dtCrowdAgent *crowd_agent, dtCrowd *crowd, int index, dtNavMeshQuery *query, DetourInputGeometry *geom, int crowd_index);
    void set_filter(int filter_index);
    int get_filter_index() const;
    int get_crowd_index() const;
    bool is_moving() const;
    Vector3 get_target_position() const;
    Vector3 get_position() const;
    Vector3 get_velocity() const;
    Vector3 get_desired_velocity() const;
    void add_shadow_agent(dtCrowdAgent *crowd_agent);
    void move_towards(Vector3 position);
    void sync_position(Vector3 position);
    void prepare_for_tick();
    void apply_new_target();
    void stop();
    Dictionary get_predicted_movement(Vector3 current_pos, Vector3 current_dir, int64_t position_ticks_timestamp, float max_turning_rad);
    void update(float seconds_since_last_tick);
    void destroy();

    bool loadParameterValues(Ref<DetourCrowdAgentParameters> params, Ref<FileAccess> source_file) { return load_parameter_values(params, source_file); }
    void setMainAgent(dtCrowdAgent *crowd_agent, dtCrowd *crowd, int index, dtNavMeshQuery *query, DetourInputGeometry *geom, int crowd_index) { set_main_agent(crowd_agent, crowd, index, query, geom, crowd_index); }
    int getFilterIndex() const { return get_filter_index(); }
    int getCrowdIndex() const { return get_crowd_index(); }
    bool isMoving() const { return is_moving(); }
    Vector3 getTargetPosition() const { return get_target_position(); }
    void addShadowAgent(dtCrowdAgent *crowd_agent) { add_shadow_agent(crowd_agent); }
    void moveTowards(Vector3 position) { move_towards(position); }
    void syncPosition(Vector3 position) { sync_position(position); }
    void applyNewTarget() { apply_new_target(); }
    Dictionary getPredictedMovement(Vector3 current_pos, Vector3 current_dir, int64_t position_ticks_timestamp, float max_turning_rad) { return get_predicted_movement(current_pos, current_dir, position_ticks_timestamp, max_turning_rad); }

private:
    dtCrowdAgent *_agent;
    dtCrowd *_crowd;
    int _agent_index;
    int _crowd_index;
    dtNavMeshQuery *_query;
    dtQueryFilter *_filter;
    int _filter_index;
    DetourInputGeometry *_input_geom;
    std::vector<dtCrowdAgent *> _shadows;

    Vector3 _position;
    Vector3 _velocity;
    Vector3 _target_position;
    Vector3 _external_position;
    std::atomic_bool _has_new_target;
    bool _has_pending_external_position;
    DetourCrowdAgentState _state;
    int _movement_mode;

    bool _is_moving;
    float _last_distance_to_target;
    float _distance_total;
    float _distance_time;
    Vector3 _last_position;
    float _movement_time;
    float _movement_over_time;

    std::chrono::system_clock::time_point last_update_time;
    mutable std::mutex _state_mutex;
};

inline Vector3 DetourCrowdAgentParameters::get_position() const { return position; }
inline void DetourCrowdAgentParameters::set_position(const Vector3 &p_value) { position = p_value; }
inline float DetourCrowdAgentParameters::get_radius() const { return radius; }
inline void DetourCrowdAgentParameters::set_radius(float p_value) { radius = p_value; }
inline float DetourCrowdAgentParameters::get_height() const { return height; }
inline void DetourCrowdAgentParameters::set_height(float p_value) { height = p_value; }
inline float DetourCrowdAgentParameters::get_max_acceleration() const { return maxAcceleration; }
inline void DetourCrowdAgentParameters::set_max_acceleration(float p_value) { maxAcceleration = p_value; }
inline float DetourCrowdAgentParameters::get_max_speed() const { return maxSpeed; }
inline void DetourCrowdAgentParameters::set_max_speed(float p_value) { maxSpeed = p_value; }
inline String DetourCrowdAgentParameters::get_filter_name() const { return filterName; }
inline void DetourCrowdAgentParameters::set_filter_name(const String &p_value) { filterName = p_value; }
inline bool DetourCrowdAgentParameters::get_anticipate_turns() const { return anticipateTurns; }
inline void DetourCrowdAgentParameters::set_anticipate_turns(bool p_value) { anticipateTurns = p_value; }
inline bool DetourCrowdAgentParameters::get_optimize_visibility() const { return optimizeVisibility; }
inline void DetourCrowdAgentParameters::set_optimize_visibility(bool p_value) { optimizeVisibility = p_value; }
inline bool DetourCrowdAgentParameters::get_optimize_topology() const { return optimizeTopology; }
inline void DetourCrowdAgentParameters::set_optimize_topology(bool p_value) { optimizeTopology = p_value; }
inline bool DetourCrowdAgentParameters::get_avoid_obstacles() const { return avoidObstacles; }
inline void DetourCrowdAgentParameters::set_avoid_obstacles(bool p_value) { avoidObstacles = p_value; }
inline bool DetourCrowdAgentParameters::get_avoid_other_agents() const { return avoidOtherAgents; }
inline void DetourCrowdAgentParameters::set_avoid_other_agents(bool p_value) { avoidOtherAgents = p_value; }
inline int DetourCrowdAgentParameters::get_obstacle_avoidance() const { return obstacleAvoidance; }
inline void DetourCrowdAgentParameters::set_obstacle_avoidance(int p_value) { obstacleAvoidance = p_value; }
inline float DetourCrowdAgentParameters::get_separation_weight() const { return separationWeight; }
inline void DetourCrowdAgentParameters::set_separation_weight(float p_value) { separationWeight = p_value; }
inline int DetourCrowdAgentParameters::get_movement_mode() const { return movementMode; }
inline void DetourCrowdAgentParameters::set_movement_mode(int p_value) { movementMode = p_value; }
inline int DetourCrowdAgent::get_filter_index() const { return _filter_index; }
inline int DetourCrowdAgent::get_crowd_index() const { return _crowd_index; }

} // namespace godot

#endif // DETOURCROWDAGENT_H

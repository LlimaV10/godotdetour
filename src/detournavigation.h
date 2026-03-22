#ifndef DETOURNAVIGATION_H
#define DETOURNAVIGATION_H

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <atomic>
#include <map>
#include <vector>

#include "detourcrowdagent.h"
#include "detournavigationmesh.h"

class DetourInputGeometry;
class GodotDetourDebugDraw;
class RecastContext;

namespace std {
class mutex;
class thread;
}

namespace godot {

class DetourObstacle;

class DetourNavigationParameters : public RefCounted {
    GDCLASS(DetourNavigationParameters, RefCounted)

public:
    static void _bind_methods();

    void set_nav_mesh_parameters(const Array &p_value);
    Array get_nav_mesh_parameters() const;
    void set_ticks_per_second(int p_value);
    int get_ticks_per_second() const;
    void set_max_obstacles(int p_value);
    int get_max_obstacles() const;
    void set_default_area_type(int p_value);
    int get_default_area_type() const;

    Array navMeshParameters;
    int ticksPerSecond = 60;
    int maxObstacles = 256;
    int defaultAreaType = 0;
};

class DetourNavigation : public RefCounted {
    GDCLASS(DetourNavigation, RefCounted)

public:
    static void _bind_methods();

    DetourNavigation();
    ~DetourNavigation();

    bool is_initialized() const;
    bool initialize(const Variant &input_mesh_instance, const Ref<DetourNavigationParameters> &parameters);
    void tick(float delta_seconds = -1.0f);
    void rebuild_changed_tiles();
    int mark_convex_area(Array vertices, float height, unsigned int area_type);
    void remove_convex_area_marker(int id);
    int add_off_mesh_connection(Vector3 from, Vector3 to, bool bidirectional, float radius, int area_type);
    void remove_off_mesh_connection(int id);
    bool set_query_filter(int index, String name, Dictionary weights);
    Ref<DetourCrowdAgent> add_agent(const Ref<DetourCrowdAgentParameters> &parameters);
    void remove_agent(const Ref<DetourCrowdAgent> &agent);
    Ref<DetourObstacle> add_cylinder_obstacle(Vector3 position, float radius, float height);
    Ref<DetourObstacle> add_box_obstacle(Vector3 position, Vector3 dimensions, float rotation_rad);
    MeshInstance3D *create_debug_mesh(int index, bool draw_cache_bounds);
    bool save(String path, bool compressed);
    bool load(String path, bool compressed);
    void clear();
    Array get_agents();
    Array get_obstacles();
    Array get_marked_area_ids();
    void navigation_thread_function();

private:
    DetourInputGeometry *_input_geometry;
    std::vector<DetourNavigationMesh *> _nav_meshes;
    std::vector<Ref<DetourCrowdAgent>> _agents;
    std::vector<Ref<DetourObstacle>> _obstacles;
    std::vector<int> _marked_area_ids;
    std::vector<int> _removed_marked_area_ids;
    std::vector<int> _off_mesh_connections;
    std::vector<int> _removed_off_mesh_connections;

    RecastContext *_recast_context;
    GodotDetourDebugDraw *_debug_drawer;

    bool _initialized;
    int _ticks_per_second;
    int _max_obstacles;
    int _default_area_type;

    std::thread *_navigation_thread;
    std::atomic_bool _stop_thread;
    std::mutex *_navigation_mutex;

    std::map<String, int> _query_filter_indices;
};

inline Array DetourNavigationParameters::get_nav_mesh_parameters() const { return navMeshParameters; }
inline void DetourNavigationParameters::set_nav_mesh_parameters(const Array &p_value) { navMeshParameters = p_value; }
inline int DetourNavigationParameters::get_ticks_per_second() const { return ticksPerSecond; }
inline void DetourNavigationParameters::set_ticks_per_second(int p_value) { ticksPerSecond = p_value; }
inline int DetourNavigationParameters::get_max_obstacles() const { return maxObstacles; }
inline void DetourNavigationParameters::set_max_obstacles(int p_value) { maxObstacles = p_value; }
inline int DetourNavigationParameters::get_default_area_type() const { return defaultAreaType; }
inline void DetourNavigationParameters::set_default_area_type(int p_value) { defaultAreaType = p_value; }
inline bool DetourNavigation::is_initialized() const { return _initialized; }

} // namespace godot

#endif // DETOURNAVIGATION_H

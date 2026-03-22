#ifndef DETOUROBSTACLE_H
#define DETOUROBSTACLE_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <map>
#include <vector>

class dtTileCache;

enum DetourObstacleType {
    OBSTACLE_TYPE_INVALID = -1,
    OBSTACLE_TYPE_CYLINDER,
    OBSTACLE_TYPE_BOX,
    NUM_OBSTACLE_TYPES
};

namespace godot {

class DetourObstacle : public RefCounted {
    GDCLASS(DetourObstacle, RefCounted)

public:
    static void _bind_methods();

    DetourObstacle();
    ~DetourObstacle();

    DetourObstacleType get_type() const;
    void set_position(const Vector3 &p_position);
    Vector3 get_position() const;
    void set_dimensions(const Vector3 &p_dimensions);
    Vector3 get_dimensions() const;

    void initialize(DetourObstacleType type, const Vector3 &position, const Vector3 &dimensions, float rotation_rad);
    bool save(const Ref<FileAccess> &target_file);
    bool load(const Ref<FileAccess> &source_file);
    void create_detour_obstacle(dtTileCache *cache);
    void add_reference(unsigned int ref, dtTileCache *cache);
    void move(Vector3 position);
    void destroy();
    bool is_destroyed() const;

    void createDetourObstacle(dtTileCache *cache) { create_detour_obstacle(cache); }
    void addReference(unsigned int ref, dtTileCache *cache) { add_reference(ref, cache); }
    bool isDestroyed() const { return is_destroyed(); }

private:
    DetourObstacleType _type;
    Vector3 _position;
    Vector3 _dimensions;
    float _rotationRad;
    bool _destroyed;
    std::map<dtTileCache *, unsigned int> _references;
};

inline DetourObstacleType DetourObstacle::get_type() const {
    return _type;
}

inline Vector3 DetourObstacle::get_position() const {
    return _position;
}

inline void DetourObstacle::set_position(const Vector3 &p_position) {
    _position = p_position;
}

inline Vector3 DetourObstacle::get_dimensions() const {
    return _dimensions;
}

inline void DetourObstacle::set_dimensions(const Vector3 &p_dimensions) {
    _dimensions = p_dimensions;
}

inline bool DetourObstacle::is_destroyed() const {
    return _destroyed;
}

} // namespace godot

#endif // DETOUROBSTACLE_H

#ifndef GODOTDETOURDEBUGDRAW_H
#define GODOTDETOURDEBUGDRAW_H

#include "DebugDraw.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

class GodotDetourDebugDraw : public duDebugDraw {
public:
    GodotDetourDebugDraw();
    ~GodotDetourDebugDraw();

    void setMaterial(godot::Ref<godot::Material> material);
    godot::Ref<godot::ArrayMesh> getArrayMesh();
    void clear();
    void debugDrawBox(float minx, float miny, float minz, float maxx, float maxy, float maxz, unsigned int *fcol);

    virtual unsigned int areaToCol(unsigned int area);
    virtual void depthMask(bool state);
    virtual void texture(bool state);
    virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f);
    virtual void vertex(const float *pos, unsigned int color);
    virtual void vertex(float x, float y, float z, unsigned int color);
    virtual void vertex(const float *pos, unsigned int color, const float *uv);
    virtual void vertex(float x, float y, float z, unsigned int color, float u, float v);
    virtual void end();

private:
    struct DebugSurfaceData {
        godot::PackedVector3Array vertices;
        godot::PackedColorArray colors;
        godot::PackedVector2Array uvs;
        bool has_uv = false;
    };

    void append_vertex(const godot::Vector3 &position, const godot::Color &color, const godot::Vector2 *uv = nullptr);
    godot::Ref<godot::ArrayMesh> build_array_mesh();
    DebugSurfaceData *get_current_surface();

    godot::Ref<godot::StandardMaterial3D> _material;
    godot::Ref<godot::ArrayMesh> _array_mesh;
    DebugSurfaceData _points;
    DebugSurfaceData _lines;
    DebugSurfaceData _tris;
    duDebugDrawPrimitives _current_primitive = DU_DRAW_TRIS;
};

#endif // GODOTDETOURDEBUGDRAW_H

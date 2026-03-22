#ifndef GODOTDETOURDEBUGDRAW_H
#define GODOTDETOURDEBUGDRAW_H

#include "DebugDraw.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>

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
    godot::Ref<godot::SurfaceTool> _surface_tool;
    godot::Ref<godot::StandardMaterial3D> _material;
    godot::Ref<godot::ArrayMesh> _array_mesh;
};

inline godot::Ref<godot::ArrayMesh> GodotDetourDebugDraw::getArrayMesh() {
    return _array_mesh;
}

#endif // GODOTDETOURDEBUGDRAW_H

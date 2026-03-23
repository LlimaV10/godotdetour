#include "godotdetourdebugdraw.h"
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include "navigationmeshhelpers.h"

using namespace godot;

Color
godotColorFromDetourColor(unsigned int input)
{
    float colorf[3];
    duIntToCol(input, colorf);
    unsigned int alpha = (input >> 24) & 0xff;
    float r = (input) & 0xff;
    float g = (input >> 8) & 0xff;
    float b = (input >> 16) & 0xff;
    float a = (input >> 24) & 0xff;
//    Godot::print("Input color: {0}", input);
//    Godot::print("Got color a: {0} {1} {2} {3}", r, g, b, a);
//    Godot::print("Got color b: {0} {1} {2} {3}", colorf[0], colorf[1], colorf[2], alpha / 255.0f);
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, alpha / 255.0f);
}

GodotDetourDebugDraw::GodotDetourDebugDraw()
{
    // Create the material
    Ref<StandardMaterial3D> mat;
    mat.instantiate();
    mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    mat->set_flag(BaseMaterial3D::FLAG_DISABLE_AMBIENT_LIGHT, true);
    mat->set_flag(BaseMaterial3D::FLAG_DONT_RECEIVE_SHADOWS, true);
    mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);

    _material = mat;
}

GodotDetourDebugDraw::~GodotDetourDebugDraw()
{
    _material.unref();
}

void
GodotDetourDebugDraw::setMaterial(Ref<Material> material)
{
    _material = material;
}

void
GodotDetourDebugDraw::clear()
{
    _array_mesh.unref();
    _points = DebugSurfaceData();
    _lines = DebugSurfaceData();
    _tris = DebugSurfaceData();
}

void
GodotDetourDebugDraw::debugDrawBox(float minx, float miny, float minz, float maxx, float maxy, float maxz, unsigned int* fcol)
{
    begin(DU_DRAW_TRIS);

    const float verts[8*3] =
    {
        minx, miny, minz, // bbl
        maxx, miny, minz, // bbr
        maxx, miny, maxz, // bfr
        minx, miny, maxz, // bfl
        minx, maxy, minz, // tbl
        maxx, maxy, minz, // tbr
        maxx, maxy, maxz, // tfr
        minx, maxy, maxz  // tfl
    };
    static const unsigned char inds[6*6] =
    {
        // back
        5, 4, 0, 5, 0, 1,
        // front
        6, 2, 3, 6, 3, 7,
        // bottom
        3, 2, 1, 3, 1, 0,
        // top
        7, 4, 5, 7, 5, 6,
        // right
        6, 5, 2, 6, 2, 1,
        // left
        7, 3, 0, 7, 0, 4
    };

    const unsigned char* in = inds;
    for (int i = 0; i < 6; ++i)
    {
        vertex(&verts[*in*3], fcol[i]); in++;
        vertex(&verts[*in*3], fcol[i]); in++;
        vertex(&verts[*in*3], fcol[i]); in++;
        vertex(&verts[*in*3], fcol[i]); in++;
        vertex(&verts[*in*3], fcol[i]); in++;
        vertex(&verts[*in*3], fcol[i]); in++;
    }

    end();
}

unsigned int
GodotDetourDebugDraw::areaToCol(unsigned int area)
{
    switch(area)
    {
        // Ground (0) : light blue
        case POLY_AREA_GROUND:
        return duRGBA(0, 192, 255, 255);
        // Water : blue
        case POLY_AREA_WATER:
        return duRGBA(0, 0, 255, 255);
        // Road : brown
        case POLY_AREA_ROAD:
        return duRGBA(50, 20, 12, 255);
        // Door : cyan
        case POLY_AREA_DOOR:
        return duRGBA(0, 255, 255, 255);
        // Grass : green
        case POLY_AREA_GRASS:
        return duRGBA(0, 255, 0, 255);
        // Jump : yellow
        case POLY_AREA_JUMP:
        return duRGBA(255, 255, 0, 255);
        // Unexpected : red
        default: return duRGBA(255, 0, 0, 255);
    }
}

void
GodotDetourDebugDraw::depthMask(bool state)
{
    // Not needed as we are creating a mesh, not directly drawing to the output
}

void
GodotDetourDebugDraw::texture(bool state)
{
    // We always use a texture as we use the SurfaceTool, which requires some settings to be set via material
}

void
GodotDetourDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
    (void)size;
    _current_primitive = prim;
}

void
GodotDetourDebugDraw::vertex(const float* pos, unsigned int color)
{
    append_vertex(Vector3(pos[0], pos[1], pos[2]), godotColorFromDetourColor(color));
}

void
GodotDetourDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
    append_vertex(Vector3(x, y, z), godotColorFromDetourColor(color));
}

void
GodotDetourDebugDraw::vertex(const float* pos, unsigned int color, const float* uv)
{
    const Vector2 uv_value(uv[0], uv[1]);
    append_vertex(Vector3(pos[0], pos[1], pos[2]), godotColorFromDetourColor(color), &uv_value);
}

void
GodotDetourDebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
    const Vector2 uv_value(u, v);
    append_vertex(Vector3(x, y, z), godotColorFromDetourColor(color), &uv_value);
}

void
GodotDetourDebugDraw::end()
{
}

void
GodotDetourDebugDraw::append_vertex(const Vector3 &position, const Color &color, const Vector2 *uv)
{
    DebugSurfaceData *surface = get_current_surface();
    surface->vertices.push_back(position);
    surface->colors.push_back(color);
    if (uv != nullptr)
    {
        surface->has_uv = true;
        surface->uvs.push_back(*uv);
    }
    else if (surface->has_uv)
    {
        surface->uvs.push_back(Vector2());
    }
}

GodotDetourDebugDraw::DebugSurfaceData *
GodotDetourDebugDraw::get_current_surface()
{
    switch (_current_primitive)
    {
        case DU_DRAW_POINTS:
            return &_points;
        case DU_DRAW_LINES:
            return &_lines;
        case DU_DRAW_TRIS:
            return &_tris;
        case DU_DRAW_QUADS:
            WARN_PRINT("Trying to use primitive type quad. Not supported by Godot. Falling back to triangles.");
            return &_tris;
        default:
            return &_tris;
    }
}

Ref<ArrayMesh>
GodotDetourDebugDraw::build_array_mesh()
{
    Ref<ArrayMesh> mesh;
    mesh.instantiate();

    auto add_surface = [&](const DebugSurfaceData &surface, Mesh::PrimitiveType primitive) {
        if (surface.vertices.is_empty())
        {
            return;
        }

        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
        arrays[Mesh::ARRAY_COLOR] = surface.colors;
        if (surface.has_uv)
        {
            arrays[Mesh::ARRAY_TEX_UV] = surface.uvs;
        }

        mesh->add_surface_from_arrays(primitive, arrays);
        mesh->surface_set_material(mesh->get_surface_count() - 1, _material->duplicate(true));
    };

    add_surface(_points, Mesh::PRIMITIVE_POINTS);
    add_surface(_lines, Mesh::PRIMITIVE_LINES);
    add_surface(_tris, Mesh::PRIMITIVE_TRIANGLES);
    return mesh;
}

Ref<ArrayMesh> GodotDetourDebugDraw::getArrayMesh() {
    if (_array_mesh.is_null())
    {
        _array_mesh = build_array_mesh();
    }
    return _array_mesh;
}

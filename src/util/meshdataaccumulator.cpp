#include "meshdataaccumulator.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "godotgeometryparser.h"

using namespace godot;

#define MDA_SAVE_VERSION 1

MeshDataAccumulator::MeshDataAccumulator(MeshInstance3D *meshInstance)
{
    GodotGeometryParser parser;
    parser.getNodeVerticesAndIndices(meshInstance, _vertices, _triangles);

    UtilityFunctions::print("Got vertices and triangles...");

    // Copy normals (we can't just copy them from the MeshDataTool since we operate on transformed values)
    // Code below mostly taken from recastnavigation sample
    size_t indexCount = _triangles.size();
    _normals.resize(indexCount);
    for (int j = 0; j < indexCount; j += 3)
    {
        const float* v0 = &_vertices[_triangles[j + 0] * 3];
        const float* v1 = &_vertices[_triangles[j + 1] * 3];
        const float* v2 = &_vertices[_triangles[j + 2] * 3];
        float e0[3], e1[3];
        for (int k = 0; k < 3; ++k)
        {
            e0[k] = v1[k] - v0[k];
            e1[k] = v2[k] - v0[k];
        }
        float* n = &_normals.data()[j];
        n[0] = e0[1]*e1[2] - e0[2]*e1[1];
        n[1] = e0[2]*e1[0] - e0[0]*e1[2];
        n[2] = e0[0]*e1[1] - e0[1]*e1[0];
        float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (d > 0)
        {
            d = 1.0f/d;
            n[0] *= d;
            n[1] *= d;
            n[2] *= d;
        }
    }
    UtilityFunctions::print("Got normals...");
}

MeshDataAccumulator::MeshDataAccumulator()
{

}


MeshDataAccumulator::~MeshDataAccumulator()
{

}

void
MeshDataAccumulator::clear()
{
    _vertices.clear();
    _triangles.clear();
    _normals.clear();
}

void
MeshDataAccumulator::append(const MeshDataAccumulator &other)
{
    const int vertex_offset = getVertCount();
    _vertices.insert(_vertices.end(), other._vertices.begin(), other._vertices.end());
    _normals.insert(_normals.end(), other._normals.begin(), other._normals.end());
    _triangles.reserve(_triangles.size() + other._triangles.size());
    for (int index : other._triangles)
    {
        _triangles.push_back(index + vertex_offset);
    }
}

void
MeshDataAccumulator::save(const Ref<FileAccess> &targetFile)
{
    // Store version
    targetFile->store_16(MDA_SAVE_VERSION);

    // Store vertices
    targetFile->store_32(_vertices.size());
    for(int i = 0; i < _vertices.size(); ++i)
    {
        targetFile->store_float(_vertices[i]);
    }

    // Store triangles
    targetFile->store_32(_triangles.size());
    for(int i = 0; i < _triangles.size(); ++i)
    {
        targetFile->store_32(_triangles[i]);
    }

    // Store normals
    targetFile->store_32(_normals.size());
    for (int i = 0; i < _normals.size(); ++i)
    {
        targetFile->store_float(_normals[i]);
    }
}

bool
MeshDataAccumulator::load(const Ref<FileAccess> &sourceFile)
{
    // Load version
    int version = sourceFile->get_16();

    // Newest version
    if (version == MDA_SAVE_VERSION)
    {
        // Vertices
        int size = sourceFile->get_32();
        _vertices.resize(size);
        for (int i = 0; i < size; ++i)
        {
            _vertices[i] = sourceFile->get_float();
        }

        // Triangles
        size = sourceFile->get_32();
        _triangles.resize(size);
        for (int i = 0; i < size; ++i)
        {
            _triangles[i] = sourceFile->get_32();
        }

        // Normals
        size = sourceFile->get_32();
        _normals.resize(size);
        for (int i = 0; i < size; ++i)
        {
            _normals[i] = sourceFile->get_float();
        }
    }
    else {
        ERR_PRINT(String("MeshDataAccumulator: Unknown save version: {0}").format(Array::make(version)));
        return false;
    }

    return true;
}

/**
  * This code is mostly taken from recastnavigation's demo project. Just slightly adjusted to fit within godotdetour.
  * Most thanks go to Mikko Mononen and maintainers for this.
  */
//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//


#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <algorithm>
#include <godot_cpp/classes/file_access.hpp>
#include "Recast.h"
#include "detourinputgeometry.h"
#include "chunkytrimesh.h"
#include "meshdataaccumulator.h"
#include "DebugDraw.h"
#include "RecastDebugDraw.h"
#include "DetourNavMesh.h"
// #include "Sample.h"

using namespace godot;

#define GEOM_SAVE_DATA_VERSION 2

static bool
intersectSegmentTriangle(const float* sp, const float* sq,
                                     const float* a, const float* b, const float* c,
                                     float &t)
{
    float v, w;
    float ab[3], ac[3], qp[3], ap[3], norm[3], e[3];
    rcVsub(ab, b, a);
    rcVsub(ac, c, a);
    rcVsub(qp, sp, sq);

    // Compute triangle normal. Can be precalculated or cached if
    // intersecting multiple segments against the same triangle
    rcVcross(norm, ab, ac);

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = rcVdot(qp, norm);
    if (d <= 0.0f) return false;

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    rcVsub(ap, sp, a);
    t = rcVdot(ap, norm);
    if (t < 0.0f) return false;
    if (t > d) return false; // For segment; exclude this code line for a ray test

    // Compute barycentric coordinate components and test if within bounds
    rcVcross(e, qp, ap);
    v = rcVdot(ac, e);
    if (v < 0.0f || v > d) return false;
    w = -rcVdot(ab, e);
    if (w < 0.0f || v + w > d) return false;

    // Segment/ray intersects triangle. Perform delayed division
    t /= d;

    return true;
}

static char*
parseRow(char* buf, char* bufEnd, char* row, int len)
{
    bool start = true;
    bool done = false;
    int n = 0;
    while (!done && buf < bufEnd)
    {
        char c = *buf;
        buf++;
        // multirow
        switch (c)
        {
            case '\n':
                if (start) break;
                done = true;
                break;
            case '\r':
                break;
            case '\t':
            case ' ':
                if (start) break;
                // else falls through
            default:
                start = false;
                row[n++] = c;
                if (n >= len-1)
                    done = true;
                break;
        }
    }
    row[n] = '\0';
    return buf;
}



DetourInputGeometry::DetourInputGeometry() :
    m_chunkyMesh(0),
    m_mesh(0),
    m_offMeshConCount(0),
    m_volumeCount(0),
    m_nextChunkId(1)
{
    memset(m_meshBMin, 0, sizeof(m_meshBMin));
    memset(m_meshBMax, 0, sizeof(m_meshBMax));
    memset(m_navMeshBMin, 0, sizeof(m_navMeshBMin));
    memset(m_navMeshBMax, 0, sizeof(m_navMeshBMax));
}

DetourInputGeometry::~DetourInputGeometry()
{
    if (m_chunkyMesh) delete m_chunkyMesh;
    if (m_mesh) delete m_mesh;
    clearChunks();
}

bool
DetourInputGeometry::loadMesh(rcContext* ctx, godot::MeshInstance3D* inputMesh)
{
    clearData();
    return addMeshChunk(ctx, inputMesh) >= 0;
}

int
DetourInputGeometry::addMeshChunk(rcContext *ctx, godot::MeshInstance3D *inputMesh)
{
    MeshDataAccumulator *chunk_mesh = new MeshDataAccumulator(inputMesh);
    if (chunk_mesh->getVertCount() == 0 || chunk_mesh->getTriCount() == 0)
    {
        delete chunk_mesh;
        ERR_PRINT("DetourInputGeometry: Source chunk does not contain triangle geometry.");
        return -1;
    }

    GeometryChunk chunk;
    chunk.id = m_nextChunkId++;
    chunk.mesh = chunk_mesh;
    if (!buildChunkBounds(*chunk_mesh, chunk.bmin, chunk.bmax))
    {
        delete chunk_mesh;
        ERR_PRINT("DetourInputGeometry: Unable to calculate source chunk bounds.");
        return -1;
    }

    m_chunks[chunk.id] = chunk;
    m_changedChunks[chunk.id] = { { chunk.bmin[0], chunk.bmin[1], chunk.bmin[2] }, { chunk.bmax[0], chunk.bmax[1], chunk.bmax[2] } };
    if (!rebuildCombinedMesh(ctx))
    {
        delete m_chunks[chunk.id].mesh;
        m_chunks.erase(chunk.id);
        m_changedChunks.erase(chunk.id);
        return -1;
    }

    return chunk.id;
}

bool
DetourInputGeometry::updateMeshChunk(rcContext *ctx, int chunkId, godot::MeshInstance3D *inputMesh)
{
    auto existing = m_chunks.find(chunkId);
    if (existing == m_chunks.end())
    {
        ERR_PRINT(String("DetourInputGeometry: Unknown source chunk ID: {0}").format(Array::make(chunkId)));
        return false;
    }

    MeshDataAccumulator *replacement = new MeshDataAccumulator(inputMesh);
    if (replacement->getVertCount() == 0 || replacement->getTriCount() == 0)
    {
        delete replacement;
        ERR_PRINT("DetourInputGeometry: Updated source chunk does not contain triangle geometry.");
        return false;
    }

    GeometryChunkBounds old_bounds = { { existing->second.bmin[0], existing->second.bmin[1], existing->second.bmin[2] },
                                       { existing->second.bmax[0], existing->second.bmax[1], existing->second.bmax[2] } };
    float new_bmin[3];
    float new_bmax[3];
    if (!buildChunkBounds(*replacement, new_bmin, new_bmax))
    {
        delete replacement;
        ERR_PRINT("DetourInputGeometry: Unable to calculate updated source chunk bounds.");
        return false;
    }

    delete existing->second.mesh;
    existing->second.mesh = replacement;
    rcVcopy(existing->second.bmin, new_bmin);
    rcVcopy(existing->second.bmax, new_bmax);
    m_changedChunks[chunkId] = { { new_bmin[0], new_bmin[1], new_bmin[2] }, { new_bmax[0], new_bmax[1], new_bmax[2] } };
    m_removedChunks[chunkId] = old_bounds;

    if (!rebuildCombinedMesh(ctx))
    {
        return false;
    }

    return true;
}

bool
DetourInputGeometry::removeMeshChunk(int chunkId)
{
    auto existing = m_chunks.find(chunkId);
    if (existing == m_chunks.end())
    {
        ERR_PRINT(String("DetourInputGeometry: Unknown source chunk ID: {0}").format(Array::make(chunkId)));
        return false;
    }
    if (m_chunks.size() <= 1)
    {
        ERR_PRINT("DetourInputGeometry: Refusing to remove the last source chunk. Clear the navigation instance instead.");
        return false;
    }

    m_removedChunks[chunkId] = { { existing->second.bmin[0], existing->second.bmin[1], existing->second.bmin[2] },
                                 { existing->second.bmax[0], existing->second.bmax[1], existing->second.bmax[2] } };
    delete existing->second.mesh;
    m_chunks.erase(existing);
    return rebuildCombinedMesh(nullptr);
}

bool
DetourInputGeometry::hasChunks() const
{
    return !m_chunks.empty();
}

Array
DetourInputGeometry::getChunkIDs() const
{
    Array ids;
    for (const auto &entry : m_chunks)
    {
        ids.append(entry.first);
    }
    return ids;
}

void
DetourInputGeometry::freezeNavMeshBounds()
{
    m_navMeshBMin[0] = m_meshBMin[0];
    m_navMeshBMin[2] = m_meshBMin[2];
    m_navMeshBMax[0] = m_meshBMax[0];
    m_navMeshBMax[2] = m_meshBMax[2];
    m_navMeshBMin[1] = m_meshBMin[1];
    m_navMeshBMax[1] = m_meshBMax[1];
}

bool
DetourInputGeometry::isWithinFrozenNavMeshBounds(const float *bmin, const float *bmax) const
{
    const bool frozen = m_navMeshBMin[0] != m_navMeshBMax[0] || m_navMeshBMin[2] != m_navMeshBMax[2];
    if (!frozen)
    {
        return true;
    }

    const float eps = 0.001f;
    return bmin[0] >= m_navMeshBMin[0] - eps &&
           bmin[2] >= m_navMeshBMin[2] - eps &&
           bmax[0] <= m_navMeshBMax[0] + eps &&
           bmax[2] <= m_navMeshBMax[2] + eps;
}

bool
DetourInputGeometry::canUseChunkMeshWithinFrozenNavBounds(godot::MeshInstance3D *inputMesh, float *out_bmin, float *out_bmax) const
{
    MeshDataAccumulator chunk_mesh(inputMesh);
    float bmin[3];
    float bmax[3];
    if (!buildChunkBounds(chunk_mesh, bmin, bmax))
    {
        return false;
    }

    if (out_bmin)
    {
        rcVcopy(out_bmin, bmin);
    }
    if (out_bmax)
    {
        rcVcopy(out_bmax, bmax);
    }
    return isWithinFrozenNavMeshBounds(bmin, bmax);
}

void
DetourInputGeometry::clearChunkChanges()
{
    m_changedChunks.clear();
    m_removedChunks.clear();
}

void
DetourInputGeometry::clearData()
{
    if (m_chunkyMesh)
    {
        delete m_chunkyMesh;
        m_chunkyMesh = 0;
    }
    if (m_mesh)
    {
        delete m_mesh;
        m_mesh = 0;
    }
    clearChunks();
    clearChunkChanges();
    m_nextChunkId = 1;
    m_offMeshConCount = 0;
    m_volumeCount = 0;
    memset(m_meshBMin, 0, sizeof(m_meshBMin));
    memset(m_meshBMax, 0, sizeof(m_meshBMax));
    memset(m_navMeshBMin, 0, sizeof(m_navMeshBMin));
    memset(m_navMeshBMax, 0, sizeof(m_navMeshBMax));
}

bool
DetourInputGeometry::save(Ref<FileAccess> targetFile)
{
    if (m_mesh == nullptr)
    {
        ERR_PRINT("DetourInputGeometry: Unable to save. No mesh.");
        return false;
    }

    // Store input geometry version
    targetFile->store_16(GEOM_SAVE_DATA_VERSION);

    // Properties
    targetFile->store_float(m_meshBMin[0]);
    targetFile->store_float(m_meshBMin[1]);
    targetFile->store_float(m_meshBMin[2]);
    targetFile->store_float(m_meshBMax[0]);
    targetFile->store_float(m_meshBMax[1]);
    targetFile->store_float(m_meshBMax[2]);
    targetFile->store_float(m_navMeshBMin[0]);
    targetFile->store_float(m_navMeshBMin[1]);
    targetFile->store_float(m_navMeshBMin[2]);
    targetFile->store_float(m_navMeshBMax[0]);
    targetFile->store_float(m_navMeshBMax[1]);
    targetFile->store_float(m_navMeshBMax[2]);
    targetFile->store_32(m_nextChunkId);

    // Store off-mesh connections
    {
        targetFile->store_32(m_offMeshConCount);
        for (int i = 0; i < m_offMeshConCount; ++i)
        {
            targetFile->store_float(m_offMeshConRads[i]);
            targetFile->store_8(m_offMeshConDirs[i]);
            targetFile->store_8(m_offMeshConAreas[i]);
            targetFile->store_16(m_offMeshConFlags[i]);
            targetFile->store_32(m_offMeshConId[i]);
            targetFile->store_8(m_offMeshConNew[i]);

            targetFile->store_float(m_offMeshConVerts[i * 3 + 0]);
            targetFile->store_float(m_offMeshConVerts[i * 3 + 1]);
            targetFile->store_float(m_offMeshConVerts[i * 3 + 2]);
            targetFile->store_float(m_offMeshConVerts[i * 3 + 3]);
            targetFile->store_float(m_offMeshConVerts[i * 3 + 4]);
            targetFile->store_float(m_offMeshConVerts[i * 3 + 5]);
        }
    }

    // Store source chunks
    {
        targetFile->store_32(m_chunks.size());
        for (const auto &entry : m_chunks)
        {
            targetFile->store_32(entry.first);
            entry.second.mesh->save(targetFile);
        }
    }

    // Store volumes
    {
        targetFile->store_32(m_volumeCount);
        for (int i = 0; i < m_volumeCount; ++i)
        {
            ConvexVolume& vol = m_volumes[i];

            // Properties
            targetFile->store_32(vol.area);
            targetFile->store_float(vol.front);
            targetFile->store_float(vol.right);
            targetFile->store_float(vol.back);
            targetFile->store_float(vol.left);
            targetFile->store_float(vol.hmin);
            targetFile->store_float(vol.hmax);
            targetFile->store_8(vol.isNew);

            // Vertices
            targetFile->store_32(vol.nverts);
            for (int j = 0; j < vol.nverts; ++j)
            {
                targetFile->store_float(vol.verts[j * 3 + 0]);
                targetFile->store_float(vol.verts[j * 3 + 1]);
                targetFile->store_float(vol.verts[j * 3 + 2]);
            }
        }
    }

    return true;
}

bool
DetourInputGeometry::load(Ref<FileAccess> sourceFile)
{
    // Load version
    int version = sourceFile->get_16();

    if (version == GEOM_SAVE_DATA_VERSION)
    {
        clearData();

        // Properties
        m_meshBMin[0] = sourceFile->get_float();
        m_meshBMin[1] = sourceFile->get_float();
        m_meshBMin[2] = sourceFile->get_float();
        m_meshBMax[0] = sourceFile->get_float();
        m_meshBMax[1] = sourceFile->get_float();
        m_meshBMax[2] = sourceFile->get_float();
        m_navMeshBMin[0] = sourceFile->get_float();
        m_navMeshBMin[1] = sourceFile->get_float();
        m_navMeshBMin[2] = sourceFile->get_float();
        m_navMeshBMax[0] = sourceFile->get_float();
        m_navMeshBMax[1] = sourceFile->get_float();
        m_navMeshBMax[2] = sourceFile->get_float();
        m_nextChunkId = sourceFile->get_32();

        // Off-mesh connections
        {
            m_offMeshConCount = sourceFile->get_32();
            for (int i = 0; i < m_offMeshConCount; ++i)
            {
                m_offMeshConRads[i] = sourceFile->get_float();
                m_offMeshConDirs[i] = sourceFile->get_8();
                m_offMeshConAreas[i] = sourceFile->get_8();
                m_offMeshConFlags[i] = sourceFile->get_16();
                m_offMeshConId[i] = sourceFile->get_32();
                m_offMeshConNew[i] = sourceFile->get_8();

                m_offMeshConVerts[i * 3 + 0] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 1] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 2] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 3] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 4] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 5] = sourceFile->get_float();
            }
        }

        // Source chunks
        {
            int chunk_count = sourceFile->get_32();
            for (int i = 0; i < chunk_count; ++i)
            {
                GeometryChunk chunk;
                chunk.id = sourceFile->get_32();
                chunk.mesh = new MeshDataAccumulator();
                if (!chunk.mesh->load(sourceFile))
                {
                    ERR_PRINT("DetourInputGeometry: Unable to load source chunk mesh.");
                    delete chunk.mesh;
                    return false;
                }
                if (!buildChunkBounds(*chunk.mesh, chunk.bmin, chunk.bmax))
                {
                    ERR_PRINT("DetourInputGeometry: Unable to calculate loaded source chunk bounds.");
                    delete chunk.mesh;
                    return false;
                }
                m_chunks[chunk.id] = chunk;
            }
        }

        // Volumes
        {
            m_volumeCount = sourceFile->get_32();
            for (int i = 0; i < m_volumeCount; ++i)
            {
                ConvexVolume& vol = m_volumes[i];

                // Properties
                vol.area = sourceFile->get_32();
                vol.front = sourceFile->get_float();
                vol.right = sourceFile->get_float();
                vol.back = sourceFile->get_float();
                vol.left = sourceFile->get_float();
                vol.hmin = sourceFile->get_float();
                vol.hmax = sourceFile->get_float();
                vol.isNew = sourceFile->get_8();

                // Vertices
                vol.nverts = sourceFile->get_32();
                for (int j = 0; j < vol.nverts; ++j)
                {
                    vol.verts[j * 3 + 0] = sourceFile->get_float();
                    vol.verts[j * 3 + 1] = sourceFile->get_float();
                    vol.verts[j * 3 + 2] = sourceFile->get_float();
                }
            }
        }

        if (!rebuildCombinedMesh(nullptr))
        {
            ERR_PRINT("DetourInputGeometry: Unable to rebuild combined source geometry after loading.");
            return false;
        }
    }
    else if (version == 1)
    {
        clearData();

        m_meshBMin[0] = sourceFile->get_float();
        m_meshBMin[1] = sourceFile->get_float();
        m_meshBMin[2] = sourceFile->get_float();
        m_meshBMax[0] = sourceFile->get_float();
        m_meshBMax[1] = sourceFile->get_float();
        m_meshBMax[2] = sourceFile->get_float();
        rcVcopy(m_navMeshBMin, m_meshBMin);
        rcVcopy(m_navMeshBMax, m_meshBMax);

        {
            m_offMeshConCount = sourceFile->get_32();
            for (int i = 0; i < m_offMeshConCount; ++i)
            {
                m_offMeshConRads[i] = sourceFile->get_float();
                m_offMeshConDirs[i] = sourceFile->get_8();
                m_offMeshConAreas[i] = sourceFile->get_8();
                m_offMeshConFlags[i] = sourceFile->get_16();
                m_offMeshConId[i] = sourceFile->get_32();
                m_offMeshConNew[i] = sourceFile->get_8();

                m_offMeshConVerts[i * 3 + 0] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 1] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 2] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 3] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 4] = sourceFile->get_float();
                m_offMeshConVerts[i * 3 + 5] = sourceFile->get_float();
            }
        }

        rcChunkyTriMesh *legacy_chunky = new rcChunkyTriMesh;
        legacy_chunky->maxTrisPerChunk = sourceFile->get_32();
        legacy_chunky->nnodes = sourceFile->get_32();
        legacy_chunky->nodes = new rcChunkyTriMeshNode[legacy_chunky->nnodes];
        for (int i = 0; i < legacy_chunky->nnodes; ++i)
        {
            rcChunkyTriMeshNode& node = legacy_chunky->nodes[i];
            node.bmin[0] = sourceFile->get_float();
            node.bmin[1] = sourceFile->get_float();
            node.bmin[2] = sourceFile->get_float();
            node.bmax[0] = sourceFile->get_float();
            node.bmax[1] = sourceFile->get_float();
            node.bmax[2] = sourceFile->get_float();
            node.i = sourceFile->get_32();
            node.n = sourceFile->get_32();
        }
        legacy_chunky->ntris = sourceFile->get_32();
        legacy_chunky->tris = new int[legacy_chunky->ntris * 3];
        for (int i = 0; i < legacy_chunky->ntris; ++i)
        {
            legacy_chunky->tris[i * 3 + 0] = sourceFile->get_32();
            legacy_chunky->tris[i * 3 + 1] = sourceFile->get_32();
            legacy_chunky->tris[i * 3 + 2] = sourceFile->get_32();
        }

        MeshDataAccumulator *legacy_mesh = new MeshDataAccumulator();
        if (!legacy_mesh->load(sourceFile))
        {
            delete legacy_chunky;
            delete legacy_mesh;
            ERR_PRINT("DetourInputGeometry: Unable to load legacy mesh.");
            return false;
        }

        {
            m_volumeCount = sourceFile->get_32();
            for (int i = 0; i < m_volumeCount; ++i)
            {
                ConvexVolume& vol = m_volumes[i];
                vol.area = sourceFile->get_32();
                vol.front = sourceFile->get_float();
                vol.right = sourceFile->get_float();
                vol.back = sourceFile->get_float();
                vol.left = sourceFile->get_float();
                vol.hmin = sourceFile->get_float();
                vol.hmax = sourceFile->get_float();
                vol.isNew = sourceFile->get_8();
                vol.nverts = sourceFile->get_32();
                for (int j = 0; j < vol.nverts; ++j)
                {
                    vol.verts[j * 3 + 0] = sourceFile->get_float();
                    vol.verts[j * 3 + 1] = sourceFile->get_float();
                    vol.verts[j * 3 + 2] = sourceFile->get_float();
                }
            }
        }

        GeometryChunk chunk;
        chunk.id = 1;
        chunk.mesh = legacy_mesh;
        rcVcopy(chunk.bmin, m_meshBMin);
        rcVcopy(chunk.bmax, m_meshBMax);
        m_chunks[chunk.id] = chunk;
        m_nextChunkId = 2;

        if (!rebuildCombinedMesh(nullptr))
        {
            delete legacy_chunky;
            ERR_PRINT("DetourInputGeometry: Unable to rebuild combined source geometry from legacy data.");
            return false;
        }
        delete legacy_chunky;
    }
    else {
        ERR_PRINT(String("DetourInputGeometry: Unknown save data version: {0}").format(Array::make(version)));
        return false;
    }

    return true;
}

bool
DetourInputGeometry::rebuildCombinedMesh(rcContext *ctx)
{
    (void)ctx;

    if (m_chunkyMesh)
    {
        delete m_chunkyMesh;
        m_chunkyMesh = 0;
    }
    if (m_mesh)
    {
        delete m_mesh;
        m_mesh = 0;
    }

    if (m_chunks.empty())
    {
        return true;
    }

    m_mesh = new MeshDataAccumulator();
    for (const auto &entry : m_chunks)
    {
        m_mesh->append(*entry.second.mesh);
    }

    if (m_mesh->getVertCount() == 0 || m_mesh->getTriCount() == 0)
    {
        ERR_PRINT("DetourInputGeometry: Combined source geometry is empty.");
        return false;
    }

    rcCalcBounds(m_mesh->getVerts(), m_mesh->getVertCount(), m_meshBMin, m_meshBMax);
    m_navMeshBMin[1] = m_meshBMin[1];
    m_navMeshBMax[1] = m_meshBMax[1];

    m_chunkyMesh = new rcChunkyTriMesh;
    if (!m_chunkyMesh)
    {
        ERR_PRINT("Out of memory 'm_chunkyMesh'.");
        return false;
    }
    if (!rcCreateChunkyTriMesh(m_mesh->getVerts(), m_mesh->getTris(), m_mesh->getTriCount(), 256, m_chunkyMesh))
    {
        ERR_PRINT("Failed to build chunky mesh.");
        return false;
    }

    return true;
}

bool
DetourInputGeometry::buildChunkBounds(const MeshDataAccumulator &mesh, float *bmin, float *bmax) const
{
    if (mesh.getVertCount() == 0)
    {
        return false;
    }
    rcCalcBounds(mesh.getVerts(), mesh.getVertCount(), bmin, bmax);
    return true;
}

void
DetourInputGeometry::clearChunks()
{
    for (auto &entry : m_chunks)
    {
        delete entry.second.mesh;
    }
    m_chunks.clear();
}

static bool
isectSegAABB(const float* sp, const float* sq,
                         const float* amin, const float* amax,
                         float& tmin, float& tmax)
{
    static const float EPS = 1e-6f;

    float d[3];
    d[0] = sq[0] - sp[0];
    d[1] = sq[1] - sp[1];
    d[2] = sq[2] - sp[2];
    tmin = 0.0;
    tmax = 1.0f;

    for (int i = 0; i < 3; i++)
    {
        if (fabsf(d[i]) < EPS)
        {
            if (sp[i] < amin[i] || sp[i] > amax[i])
                return false;
        }
        else
        {
            const float ood = 1.0f / d[i];
            float t1 = (amin[i] - sp[i]) * ood;
            float t2 = (amax[i] - sp[i]) * ood;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }

    return true;
}


bool
DetourInputGeometry::raycastMesh(float* src, float* dst, float& tmin)
{
    float dir[3];
    rcVsub(dir, dst, src);

    // Prune hit ray.
    float btmin, btmax;
    if (!isectSegAABB(src, dst, m_meshBMin, m_meshBMax, btmin, btmax))
        return false;
    float p[2], q[2];
    p[0] = src[0] + (dst[0]-src[0])*btmin;
    p[1] = src[2] + (dst[2]-src[2])*btmin;
    q[0] = src[0] + (dst[0]-src[0])*btmax;
    q[1] = src[2] + (dst[2]-src[2])*btmax;

    int cid[512];
    const int ncid = rcGetChunksOverlappingSegment(m_chunkyMesh, p, q, cid, 512);
    if (!ncid)
        return false;

    tmin = 1.0f;
    bool hit = false;
    const float* verts = m_mesh->getVerts();

    for (int i = 0; i < ncid; ++i)
    {
        const rcChunkyTriMeshNode& node = m_chunkyMesh->nodes[cid[i]];
        const int* tris = &m_chunkyMesh->tris[node.i*3];
        const int ntris = node.n;

        for (int j = 0; j < ntris*3; j += 3)
        {
            float t = 1;
            if (intersectSegmentTriangle(src, dst,
                                         &verts[tris[j]*3],
                                         &verts[tris[j+1]*3],
                                         &verts[tris[j+2]*3], t))
            {
                if (t < tmin)
                    tmin = t;
                hit = true;
            }
        }
    }

    return hit;
}

void
DetourInputGeometry::addOffMeshConnection(const float* spos, const float* epos, const float rad,
                                     unsigned char bidir, unsigned char area, unsigned short flags)
{
    if (m_offMeshConCount >= MAX_OFFMESH_CONNECTIONS) return;
    float* v = &m_offMeshConVerts[m_offMeshConCount*3*2];
    m_offMeshConRads[m_offMeshConCount] = rad;
    m_offMeshConDirs[m_offMeshConCount] = bidir;
    m_offMeshConAreas[m_offMeshConCount] = area;
    m_offMeshConFlags[m_offMeshConCount] = flags;
    m_offMeshConId[m_offMeshConCount] = 1000 + m_offMeshConCount;
    m_offMeshConNew[m_offMeshConCount] = true;
    rcVcopy(&v[0], spos);
    rcVcopy(&v[3], epos);
    m_offMeshConCount++;
}

void
DetourInputGeometry::deleteOffMeshConnection(int i)
{
    m_offMeshConCount--;
    float* src = &m_offMeshConVerts[m_offMeshConCount*3*2];
    float* dst = &m_offMeshConVerts[i*3*2];
    rcVcopy(&dst[0], &src[0]);
    rcVcopy(&dst[3], &src[3]);
    m_offMeshConRads[i] = m_offMeshConRads[m_offMeshConCount];
    m_offMeshConDirs[i] = m_offMeshConDirs[m_offMeshConCount];
    m_offMeshConAreas[i] = m_offMeshConAreas[m_offMeshConCount];
    m_offMeshConFlags[i] = m_offMeshConFlags[m_offMeshConCount];
}

void
DetourInputGeometry::drawOffMeshConnections(duDebugDraw* dd, bool hilight)
{
    unsigned int conColor = duRGBA(192,0,128,192);
    unsigned int baseColor = duRGBA(0,0,0,64);
    dd->depthMask(false);

    dd->begin(DU_DRAW_LINES, 2.0f);
    for (int i = 0; i < m_offMeshConCount; ++i)
    {
        float* v = &m_offMeshConVerts[i*3*2];

        dd->vertex(v[0],v[1],v[2], baseColor);
        dd->vertex(v[0],v[1]+0.2f,v[2], baseColor);

        dd->vertex(v[3],v[4],v[5], baseColor);
        dd->vertex(v[3],v[4]+0.2f,v[5], baseColor);

        duAppendCircle(dd, v[0],v[1]+0.1f,v[2], m_offMeshConRads[i], baseColor);
        duAppendCircle(dd, v[3],v[4]+0.1f,v[5], m_offMeshConRads[i], baseColor);

        if (hilight)
        {
            duAppendArc(dd, v[0],v[1],v[2], v[3],v[4],v[5], 0.25f,
                        (m_offMeshConDirs[i]&1) ? 0.6f : 0.0f, 0.6f, conColor);
        }
    }
    dd->end();

    dd->depthMask(true);
}

void
DetourInputGeometry::addConvexVolume(const float* verts, const int nverts,
                                const float minh, const float maxh, unsigned char area)
{
    if (m_volumeCount >= MAX_VOLUMES) return;
    ConvexVolume* vol = &m_volumes[m_volumeCount++];
    memset(vol, 0, sizeof(ConvexVolume));
    memcpy(vol->verts, verts, sizeof(float)*3*nverts);
    vol->hmin = minh;
    vol->hmax = maxh;
    vol->nverts = nverts;
    vol->area = area;

    // Create top/bottom/left/right of this convex volume
    vol->left = 1000000.0f;
    vol->right = -1000000.0f;
    vol->front = 1000000.0f;
    vol->back = -1000000.0f;
    for (int i = 0; i < nverts; ++i)
    {
        float x = verts[i*3 + 0];
        float z = verts[i*3 + 2];
        if (x < vol->left) vol->left = x;
        if (x > vol->right) vol->right = x;
        if (z < vol->front) vol->front = z;
        if (z > vol->back) vol->back = z;
    }
    vol->isNew = true;
}

void
DetourInputGeometry::deleteConvexVolume(int i)
{
    m_volumeCount--;
    m_volumes[i] = m_volumes[m_volumeCount];
}

void
DetourInputGeometry::drawConvexVolumes(struct duDebugDraw* dd, bool /*hilight*/)
{
    dd->depthMask(false);

    dd->begin(DU_DRAW_TRIS);

    for (int i = 0; i < m_volumeCount; ++i)
    {
        const ConvexVolume* vol = &m_volumes[i];
        unsigned int col = duTransCol(dd->areaToCol(vol->area), 128);
        for (int j = 0, k = vol->nverts-1; j < vol->nverts; k = j++)
        {
            const float* va = &vol->verts[k*3];
            const float* vb = &vol->verts[j*3];

            dd->vertex(vol->verts[0],vol->hmax,vol->verts[2], col);
            dd->vertex(vb[0],vol->hmax,vb[2], col);
            dd->vertex(va[0],vol->hmax,va[2], col);

            dd->vertex(va[0],vol->hmin,va[2], duDarkenCol(col));
            dd->vertex(va[0],vol->hmax,va[2], col);
            dd->vertex(vb[0],vol->hmax,vb[2], col);

            dd->vertex(va[0],vol->hmin,va[2], duDarkenCol(col));
            dd->vertex(vb[0],vol->hmax,vb[2], col);
            dd->vertex(vb[0],vol->hmin,vb[2], duDarkenCol(col));
        }
    }

    dd->end();

    dd->begin(DU_DRAW_LINES, 2.0f);
    for (int i = 0; i < m_volumeCount; ++i)
    {
        const ConvexVolume* vol = &m_volumes[i];
        unsigned int col = duTransCol(dd->areaToCol(vol->area), 220);
        for (int j = 0, k = vol->nverts-1; j < vol->nverts; k = j++)
        {
            const float* va = &vol->verts[k*3];
            const float* vb = &vol->verts[j*3];
            dd->vertex(va[0],vol->hmin,va[2], duDarkenCol(col));
            dd->vertex(vb[0],vol->hmin,vb[2], duDarkenCol(col));
            dd->vertex(va[0],vol->hmax,va[2], col);
            dd->vertex(vb[0],vol->hmax,vb[2], col);
            dd->vertex(va[0],vol->hmin,va[2], duDarkenCol(col));
            dd->vertex(va[0],vol->hmax,va[2], col);
        }
    }
    dd->end();

    dd->begin(DU_DRAW_POINTS, 3.0f);
    for (int i = 0; i < m_volumeCount; ++i)
    {
        const ConvexVolume* vol = &m_volumes[i];
        unsigned int col = duDarkenCol(duTransCol(dd->areaToCol(vol->area), 220));
        for (int j = 0; j < vol->nverts; ++j)
        {
            dd->vertex(vol->verts[j*3+0],vol->verts[j*3+1]+0.1f,vol->verts[j*3+2], col);
            dd->vertex(vol->verts[j*3+0],vol->hmin,vol->verts[j*3+2], col);
            dd->vertex(vol->verts[j*3+0],vol->hmax,vol->verts[j*3+2], col);
        }
    }
    dd->end();


    dd->depthMask(true);
}

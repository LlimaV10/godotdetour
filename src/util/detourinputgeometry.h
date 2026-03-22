#ifndef DETOURINPUTGEOMETRY_H
#define DETOURINPUTGEOMETRY_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

#include <map>

#include "chunkytrimesh.h"

using namespace godot;
class MeshDataAccumulator;

static const int MAX_CONVEXVOL_PTS = 12;
struct ConvexVolume {
    float verts[MAX_CONVEXVOL_PTS * 3];
    float hmin, hmax;
    int nverts;
    int area;
    float front;
    float back;
    float left;
    float right;
    bool isNew = false;
};

class DetourInputGeometry {
public:
    static const int MAX_OFFMESH_CONNECTIONS = 256;
    static const int MAX_VOLUMES = 256;

    struct GeometryChunkBounds {
        float bmin[3];
        float bmax[3];
    };

private:
    struct GeometryChunk {
        int id = -1;
        MeshDataAccumulator *mesh = nullptr;
        float bmin[3] = { 0.0f, 0.0f, 0.0f };
        float bmax[3] = { 0.0f, 0.0f, 0.0f };
    };

    rcChunkyTriMesh *m_chunkyMesh;
    MeshDataAccumulator *m_mesh;
    float m_meshBMin[3], m_meshBMax[3];
    float m_navMeshBMin[3], m_navMeshBMax[3];
    float m_offMeshConVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2];
    float m_offMeshConRads[MAX_OFFMESH_CONNECTIONS];
    unsigned char m_offMeshConDirs[MAX_OFFMESH_CONNECTIONS];
    unsigned char m_offMeshConAreas[MAX_OFFMESH_CONNECTIONS];
    unsigned short m_offMeshConFlags[MAX_OFFMESH_CONNECTIONS];
    unsigned int m_offMeshConId[MAX_OFFMESH_CONNECTIONS];
    bool m_offMeshConNew[MAX_OFFMESH_CONNECTIONS];
    int m_offMeshConCount;
    ConvexVolume m_volumes[MAX_VOLUMES];
    int m_volumeCount;
    int m_nextChunkId;
    std::map<int, GeometryChunk> m_chunks;
    std::map<int, GeometryChunkBounds> m_changedChunks;
    std::map<int, GeometryChunkBounds> m_removedChunks;

public:
    DetourInputGeometry();
    ~DetourInputGeometry();

    bool loadMesh(class rcContext *ctx, godot::MeshInstance3D *inputMesh);
    int addMeshChunk(class rcContext *ctx, godot::MeshInstance3D *inputMesh);
    bool updateMeshChunk(class rcContext *ctx, int chunkId, godot::MeshInstance3D *inputMesh);
    bool removeMeshChunk(int chunkId);
    bool hasChunks() const;
    godot::Array getChunkIDs() const;
    void freezeNavMeshBounds();
    bool isWithinFrozenNavMeshBounds(const float *bmin, const float *bmax) const;
    bool canUseChunkMeshWithinFrozenNavBounds(godot::MeshInstance3D *inputMesh, float *out_bmin = nullptr, float *out_bmax = nullptr) const;
    void clearChunkChanges();
    void clearData();
    bool save(Ref<FileAccess> targetFile);
    bool load(Ref<FileAccess> sourceFile);

    const MeshDataAccumulator *getMesh() const { return m_mesh; }
    const float *getMeshBoundsMin() const { return m_meshBMin; }
    const float *getMeshBoundsMax() const { return m_meshBMax; }
    const float *getNavMeshBoundsMin() const { return m_navMeshBMin; }
    const float *getNavMeshBoundsMax() const { return m_navMeshBMax; }
    const rcChunkyTriMesh *getChunkyMesh() const { return m_chunkyMesh; }
    const std::map<int, GeometryChunkBounds> &getChangedChunks() const { return m_changedChunks; }
    const std::map<int, GeometryChunkBounds> &getRemovedChunks() const { return m_removedChunks; }
    bool raycastMesh(float *src, float *dst, float &tmin);

    int getOffMeshConnectionCount() const { return m_offMeshConCount; }
    const float *getOffMeshConnectionVerts() const { return m_offMeshConVerts; }
    const float *getOffMeshConnectionRads() const { return m_offMeshConRads; }
    const unsigned char *getOffMeshConnectionDirs() const { return m_offMeshConDirs; }
    const unsigned char *getOffMeshConnectionAreas() const { return m_offMeshConAreas; }
    const unsigned short *getOffMeshConnectionFlags() const { return m_offMeshConFlags; }
    const unsigned int *getOffMeshConnectionId() const { return m_offMeshConId; }
    bool *getOffMeshConnectionNew() { return m_offMeshConNew; }
    void addOffMeshConnection(const float *spos, const float *epos, float rad, unsigned char bidir, unsigned char area, unsigned short flags);
    void deleteOffMeshConnection(int i);
    void drawOffMeshConnections(struct duDebugDraw *dd, bool hilight = false);

    int getConvexVolumeCount() const { return m_volumeCount; }
    ConvexVolume *getConvexVolumes() { return m_volumes; }
    void addConvexVolume(const float *verts, int nverts, float minh, float maxh, unsigned char area);
    void deleteConvexVolume(int i);
    void drawConvexVolumes(struct duDebugDraw *dd, bool hilight = false);

private:
    bool rebuildCombinedMesh(class rcContext *ctx);
    bool buildChunkBounds(const MeshDataAccumulator &mesh, float *bmin, float *bmax) const;
    void clearChunks();
    DetourInputGeometry(const DetourInputGeometry &);
    DetourInputGeometry &operator=(const DetourInputGeometry &);
};

#endif // DETOURINPUTGEOMETRY_H

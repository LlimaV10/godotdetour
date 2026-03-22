#ifndef DETOURINPUTGEOMETRY_H
#define DETOURINPUTGEOMETRY_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

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

private:
    rcChunkyTriMesh *m_chunkyMesh;
    MeshDataAccumulator *m_mesh;
    float m_meshBMin[3], m_meshBMax[3];
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

public:
    DetourInputGeometry();
    ~DetourInputGeometry();

    bool loadMesh(class rcContext *ctx, godot::MeshInstance3D *inputMesh);
    void clearData();
    bool save(Ref<FileAccess> targetFile);
    bool load(Ref<FileAccess> sourceFile);

    const MeshDataAccumulator *getMesh() const { return m_mesh; }
    const float *getMeshBoundsMin() const { return m_meshBMin; }
    const float *getMeshBoundsMax() const { return m_meshBMax; }
    const float *getNavMeshBoundsMin() const { return m_meshBMin; }
    const float *getNavMeshBoundsMax() const { return m_meshBMax; }
    const rcChunkyTriMesh *getChunkyMesh() const { return m_chunkyMesh; }
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
    DetourInputGeometry(const DetourInputGeometry &);
    DetourInputGeometry &operator=(const DetourInputGeometry &);
};

#endif // DETOURINPUTGEOMETRY_H

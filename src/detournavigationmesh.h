#ifndef DETOURNAVIGATIONMESH_H
#define DETOURNAVIGATIONMESH_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <map>
#include <vector>

#include "detourcrowdagent.h"

class DetourInputGeometry;
class dtCrowd;
class dtCrowdAgent;
class dtNavMesh;
class dtNavMeshQuery;
class dtTileCache;
class GodotDetourDebugDraw;
class RecastContext;
class rcConfig;
struct FastLZCompressor;
struct LinearAllocator;
struct MeshProcess;
struct TileCacheData;

namespace godot {

class DetourObstacle;

class DetourNavigationMeshParameters : public RefCounted {
    GDCLASS(DetourNavigationMeshParameters, RefCounted)

public:
    static void _bind_methods();

    void set_cell_size(const Vector2 &p_value);
    Vector2 get_cell_size() const;
    void set_max_num_agents(int p_value);
    int get_max_num_agents() const;
    void set_max_agent_slope(float p_value);
    float get_max_agent_slope() const;
    void set_max_agent_height(float p_value);
    float get_max_agent_height() const;
    void set_max_agent_climb(float p_value);
    float get_max_agent_climb() const;
    void set_max_agent_radius(float p_value);
    float get_max_agent_radius() const;
    void set_max_edge_length(float p_value);
    float get_max_edge_length() const;
    void set_max_simplification_error(float p_value);
    float get_max_simplification_error() const;
    void set_min_num_cells_per_island(int p_value);
    int get_min_num_cells_per_island() const;
    void set_min_cell_span_count(int p_value);
    int get_min_cell_span_count() const;
    void set_max_verts_per_poly(int p_value);
    int get_max_verts_per_poly() const;
    void set_tile_size(int p_value);
    int get_tile_size() const;
    void set_layers_per_tile(int p_value);
    int get_layers_per_tile() const;
    void set_detail_sample_distance(float p_value);
    float get_detail_sample_distance() const;
    void set_detail_sample_max_error(float p_value);
    float get_detail_sample_max_error() const;

    Vector2 cellSize;
    int maxNumAgents = 256;
    float maxAgentSlope = 0.0f;
    float maxAgentHeight = 0.0f;
    float maxAgentClimb = 0.0f;
    float maxAgentRadius = 0.0f;
    float maxEdgeLength = 0.0f;
    float maxSimplificationError = 0.0f;
    int minNumCellsPerIsland = 0;
    int minCellSpanCount = 0;
    int maxVertsPerPoly = 0;
    int tileSize = 0;
    int layersPerTile = 0;
    float detailSampleDistance = 0.0f;
    float detailSampleMaxError = 0.0f;
};

struct ConvexVolumeData {
    Array vertices;
    float height;
    unsigned char areaType;
};

struct ChangedTileLayerData {
    int ref;
    int layer;
    bool doAll;
};

struct ChangedTileLayers {
    std::pair<int, int> tilePos;
    std::vector<ChangedTileLayerData> layers;
};

class DetourNavigationMesh : public RefCounted {
    GDCLASS(DetourNavigationMesh, RefCounted)

public:
    static void _bind_methods();

    DetourNavigationMesh();
    ~DetourNavigationMesh();

    bool initialize(DetourInputGeometry *inputGeom, Ref<DetourNavigationMeshParameters> params, int maxObstacles, RecastContext *recastContext, int index);
    bool save(Ref<FileAccess> targetFile);
    bool load(DetourInputGeometry *inputGeom, RecastContext *recastContext, Ref<FileAccess> sourceFile);
    void rebuildChangedTiles(const std::vector<int> &removedMarkedAreaIDs, const std::vector<int> &removedOffMeshConnections);
    bool addAgent(Ref<DetourCrowdAgent> agent, Ref<DetourCrowdAgentParameters> parameters, bool main = true);
    void removeAgent(dtCrowdAgent *agent);
    void addObstacle(Ref<DetourObstacle> obstacle);
    void update(float timeDeltaSeconds);
    void createDebugMesh(GodotDetourDebugDraw *debugDrawer, bool drawCacheBounds);
    dtCrowd *getCrowd();
    float getActorFitFactor(float agentRadius, float agentHeight);

    bool initialize_crowd() { return initializeCrowd(); }
    void rebuild_changed_tiles(const std::vector<int> &removed_marked_area_ids, const std::vector<int> &removed_off_mesh_connections) { rebuildChangedTiles(removed_marked_area_ids, removed_off_mesh_connections); }
    bool add_agent(Ref<DetourCrowdAgent> agent, Ref<DetourCrowdAgentParameters> parameters, bool main = true) { return addAgent(agent, parameters, main); }
    void add_obstacle(const Ref<DetourObstacle> &obstacle) { addObstacle(obstacle); }
    void create_debug_mesh(GodotDetourDebugDraw *debugDrawer, bool drawCacheBounds) { createDebugMesh(debugDrawer, drawCacheBounds); }
    dtCrowd *get_crowd() { return getCrowd(); }
    float get_actor_fit_factor(float agentRadius, float agentHeight) { return getActorFitFactor(agentRadius, agentHeight); }

private:
    bool initializeCrowd();
    int rasterizeTileLayers(int tileX, int tileZ, const rcConfig &cfg, TileCacheData *tiles, int maxTiles);
    void debugDrawTiles(GodotDetourDebugDraw *debugDrawer);
    void debugDrawObstacles(GodotDetourDebugDraw *debugDrawer);

    RecastContext *_recastContext;
    rcConfig *_rcConfig;
    dtTileCache *_tileCache;
    dtNavMesh *_navMesh;
    dtNavMeshQuery *_navQuery;
    dtCrowd *_crowd;
    LinearAllocator *_allocator;
    FastLZCompressor *_compressor;
    MeshProcess *_meshProcess;
    DetourInputGeometry *_inputGeom;

    float _maxAgentSlope;
    float _maxAgentHeight;
    float _maxAgentClimb;
    float _maxAgentRadius;

    int _maxAgents;
    int _maxObstacles;
    int _maxLayers;
    int _navQueryMaxNodes;

    Vector2 _cellSize;
    int _tileSize;
    int _layersPerTile;

    int _navMeshIndex;

    std::map<int, ChangedTileLayers> _affectedTilesByVolume;
    std::map<int, ChangedTileLayers> _affectedTilesByConnection;
};

inline Vector2 DetourNavigationMeshParameters::get_cell_size() const { return cellSize; }
inline void DetourNavigationMeshParameters::set_cell_size(const Vector2 &p_value) { cellSize = p_value; }
inline int DetourNavigationMeshParameters::get_max_num_agents() const { return maxNumAgents; }
inline void DetourNavigationMeshParameters::set_max_num_agents(int p_value) { maxNumAgents = p_value; }
inline float DetourNavigationMeshParameters::get_max_agent_slope() const { return maxAgentSlope; }
inline void DetourNavigationMeshParameters::set_max_agent_slope(float p_value) { maxAgentSlope = p_value; }
inline float DetourNavigationMeshParameters::get_max_agent_height() const { return maxAgentHeight; }
inline void DetourNavigationMeshParameters::set_max_agent_height(float p_value) { maxAgentHeight = p_value; }
inline float DetourNavigationMeshParameters::get_max_agent_climb() const { return maxAgentClimb; }
inline void DetourNavigationMeshParameters::set_max_agent_climb(float p_value) { maxAgentClimb = p_value; }
inline float DetourNavigationMeshParameters::get_max_agent_radius() const { return maxAgentRadius; }
inline void DetourNavigationMeshParameters::set_max_agent_radius(float p_value) { maxAgentRadius = p_value; }
inline float DetourNavigationMeshParameters::get_max_edge_length() const { return maxEdgeLength; }
inline void DetourNavigationMeshParameters::set_max_edge_length(float p_value) { maxEdgeLength = p_value; }
inline float DetourNavigationMeshParameters::get_max_simplification_error() const { return maxSimplificationError; }
inline void DetourNavigationMeshParameters::set_max_simplification_error(float p_value) { maxSimplificationError = p_value; }
inline int DetourNavigationMeshParameters::get_min_num_cells_per_island() const { return minNumCellsPerIsland; }
inline void DetourNavigationMeshParameters::set_min_num_cells_per_island(int p_value) { minNumCellsPerIsland = p_value; }
inline int DetourNavigationMeshParameters::get_min_cell_span_count() const { return minCellSpanCount; }
inline void DetourNavigationMeshParameters::set_min_cell_span_count(int p_value) { minCellSpanCount = p_value; }
inline int DetourNavigationMeshParameters::get_max_verts_per_poly() const { return maxVertsPerPoly; }
inline void DetourNavigationMeshParameters::set_max_verts_per_poly(int p_value) { maxVertsPerPoly = p_value; }
inline int DetourNavigationMeshParameters::get_tile_size() const { return tileSize; }
inline void DetourNavigationMeshParameters::set_tile_size(int p_value) { tileSize = p_value; }
inline int DetourNavigationMeshParameters::get_layers_per_tile() const { return layersPerTile; }
inline void DetourNavigationMeshParameters::set_layers_per_tile(int p_value) { layersPerTile = p_value; }
inline float DetourNavigationMeshParameters::get_detail_sample_distance() const { return detailSampleDistance; }
inline void DetourNavigationMeshParameters::set_detail_sample_distance(float p_value) { detailSampleDistance = p_value; }
inline float DetourNavigationMeshParameters::get_detail_sample_max_error() const { return detailSampleMaxError; }
inline void DetourNavigationMeshParameters::set_detail_sample_max_error(float p_value) { detailSampleMaxError = p_value; }
inline dtCrowd *DetourNavigationMesh::getCrowd() { return _crowd; }

} // namespace godot

#endif // DETOURNAVIGATIONMESH_H

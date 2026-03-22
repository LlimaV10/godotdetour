#include "detournavigation.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <DetourCrowd.h>

#include <chrono>
#include <climits>
#include <mutex>
#include <thread>

#include "detourobstacle.h"
#include "util/detourinputgeometry.h"
#include "util/godotdetourdebugdraw.h"
#include "util/navigationmeshhelpers.h"
#include "util/recastcontext.h"

using namespace godot;

#define SAVE_DATA_VERSION 1

void DetourNavigationParameters::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_nav_mesh_parameters"), &DetourNavigationParameters::get_nav_mesh_parameters);
    ClassDB::bind_method(D_METHOD("set_nav_mesh_parameters", "value"), &DetourNavigationParameters::set_nav_mesh_parameters);
    ClassDB::bind_method(D_METHOD("get_ticks_per_second"), &DetourNavigationParameters::get_ticks_per_second);
    ClassDB::bind_method(D_METHOD("set_ticks_per_second", "value"), &DetourNavigationParameters::set_ticks_per_second);
    ClassDB::bind_method(D_METHOD("get_max_obstacles"), &DetourNavigationParameters::get_max_obstacles);
    ClassDB::bind_method(D_METHOD("set_max_obstacles", "value"), &DetourNavigationParameters::set_max_obstacles);
    ClassDB::bind_method(D_METHOD("get_default_area_type"), &DetourNavigationParameters::get_default_area_type);
    ClassDB::bind_method(D_METHOD("set_default_area_type", "value"), &DetourNavigationParameters::set_default_area_type);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "navMeshParameters"), "set_nav_mesh_parameters", "get_nav_mesh_parameters");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ticksPerSecond"), "set_ticks_per_second", "get_ticks_per_second");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "maxObstacles"), "set_max_obstacles", "get_max_obstacles");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "defaultAreaType"), "set_default_area_type", "get_default_area_type");
}

void DetourNavigation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "input_mesh_instance", "parameters"), &DetourNavigation::initialize);
    ClassDB::bind_method(D_METHOD("tick", "delta_seconds"), &DetourNavigation::tick, DEFVAL(-1.0f));
    ClassDB::bind_method(D_METHOD("rebuildChangedTiles"), &DetourNavigation::rebuild_changed_tiles);
    ClassDB::bind_method(D_METHOD("markConvexArea", "vertices", "height", "area_type"), &DetourNavigation::mark_convex_area);
    ClassDB::bind_method(D_METHOD("addAgent", "parameters"), &DetourNavigation::add_agent);
    ClassDB::bind_method(D_METHOD("removeAgent", "agent"), &DetourNavigation::remove_agent);
    ClassDB::bind_method(D_METHOD("addBoxObstacle", "position", "dimensions", "rotation_rad"), &DetourNavigation::add_box_obstacle);
    ClassDB::bind_method(D_METHOD("addCylinderObstacle", "position", "radius", "height"), &DetourNavigation::add_cylinder_obstacle);
    ClassDB::bind_method(D_METHOD("createDebugMesh", "index", "draw_cache_bounds"), &DetourNavigation::create_debug_mesh);
    ClassDB::bind_method(D_METHOD("setQueryFilter", "index", "name", "weights"), &DetourNavigation::set_query_filter);
    ClassDB::bind_method(D_METHOD("save", "path", "compressed"), &DetourNavigation::save);
    ClassDB::bind_method(D_METHOD("load", "path", "compressed"), &DetourNavigation::load);
    ClassDB::bind_method(D_METHOD("clear"), &DetourNavigation::clear);
    ClassDB::bind_method(D_METHOD("getAgents"), &DetourNavigation::get_agents);
    ClassDB::bind_method(D_METHOD("getObstacles"), &DetourNavigation::get_obstacles);
    ClassDB::bind_method(D_METHOD("getMarkedAreaIDs"), &DetourNavigation::get_marked_area_ids);
    ClassDB::bind_method(D_METHOD("isInitialized"), &DetourNavigation::is_initialized);
    ClassDB::bind_method(D_METHOD("addOffMeshConnection", "from", "to", "bidirectional", "radius", "area_type"), &DetourNavigation::add_off_mesh_connection);
    ClassDB::bind_method(D_METHOD("removeOffMeshConnection", "id"), &DetourNavigation::remove_off_mesh_connection);

    ADD_SIGNAL(MethodInfo("navigation_tick_done", PropertyInfo(Variant::FLOAT, "executionTimeSeconds")));
}

DetourNavigation::DetourNavigation()
        : _input_geometry(nullptr),
          _recast_context(nullptr),
          _debug_drawer(nullptr),
          _initialized(false),
          _ticks_per_second(60),
          _max_obstacles(256),
          _default_area_type(0),
          _navigation_thread(nullptr),
          _stop_thread(false),
          _navigation_mutex(nullptr) {
    _navigation_mutex = new std::mutex();
    _recast_context = new RecastContext();
    _input_geometry = new DetourInputGeometry();
}

DetourNavigation::~DetourNavigation() {
    _stop_thread = true;
    if (_navigation_thread) {
        if (_navigation_thread->joinable()) {
            _navigation_thread->join();
        }
        delete _navigation_thread;
    }
    delete _navigation_mutex;

    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        memdelete(nav_mesh);
    }

    if (_debug_drawer) {
        delete _debug_drawer;
    }

    delete _input_geometry;
    delete _recast_context;
}

bool DetourNavigation::initialize(const Variant &input_mesh_instance, const Ref<DetourNavigationParameters> &parameters) {
    if (_initialized) {
        ERR_PRINT("DetourNavigation already initialized.");
        return false;
    }

    MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(input_mesh_instance.operator Object *());
    if (mesh_instance == nullptr) {
        ERR_PRINT("Passed inputMesh must be of type MeshInstance3D.");
        return false;
    }

    Ref<Mesh> mesh_to_convert = mesh_instance->get_mesh();
    if (mesh_to_convert.is_null()) {
        ERR_PRINT("Passed MeshInstance3D does not have a mesh.");
        return false;
    }

    if (!_input_geometry->loadMesh(_recast_context, mesh_instance)) {
        ERR_PRINT("Input geometry failed to load the mesh.");
        return false;
    }

    _ticks_per_second = parameters->ticksPerSecond;
    _max_obstacles = parameters->maxObstacles;
    _default_area_type = parameters->defaultAreaType;
    for (int i = 0; i < parameters->navMeshParameters.size(); ++i) {
        Ref<DetourNavigationMeshParameters> nav_mesh_params = parameters->navMeshParameters[i];
        DetourNavigationMesh *nav_mesh = memnew(DetourNavigationMesh);
        if (!nav_mesh->initialize(_input_geometry, nav_mesh_params, _max_obstacles, _recast_context, i)) {
            ERR_PRINT("Unable to initialize detour navigation mesh!");
            memdelete(nav_mesh);
            return false;
        }
        _nav_meshes.push_back(nav_mesh);
    }

    _stop_thread = false;
    _navigation_thread = new std::thread(&DetourNavigation::navigation_thread_function, this);
    _initialized = true;
    return true;
}

void DetourNavigation::tick(float delta_seconds) {
    if (!_initialized) {
        return;
    }

    const float step = delta_seconds > 0.0f ? delta_seconds : (1.0f / static_cast<float>(_ticks_per_second));
    std::lock_guard<std::mutex> lock(*_navigation_mutex);

    for (int i = 0; i < static_cast<int>(_obstacles.size()); ++i) {
        if (_obstacles[i]->is_destroyed()) {
            _obstacles.erase(_obstacles.begin() + i);
            i--;
        }
    }

    for (const Ref<DetourCrowdAgent> &agent : _agents) {
        agent->prepare_for_tick();
    }

    for (const Ref<DetourCrowdAgent> &agent : _agents) {
        agent->apply_new_target();
    }

    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        nav_mesh->update(step);
    }

    for (const Ref<DetourCrowdAgent> &agent : _agents) {
        agent->update(step);
    }
}

void DetourNavigation::rebuild_changed_tiles() {
    _navigation_mutex->lock();
    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        nav_mesh->rebuild_changed_tiles(_removed_marked_area_ids, _removed_off_mesh_connections);
    }
    _removed_marked_area_ids.clear();
    _removed_off_mesh_connections.clear();

    for (int i = 0; i < _input_geometry->getConvexVolumeCount(); ++i) {
        _input_geometry->getConvexVolumes()[i].isNew = false;
    }
    for (int i = 0; i < _input_geometry->getOffMeshConnectionCount(); ++i) {
        _input_geometry->getOffMeshConnectionNew()[i] = false;
    }
    _navigation_mutex->unlock();
}

int DetourNavigation::mark_convex_area(Array vertices, float height, unsigned int area_type) {
    if (area_type > UCHAR_MAX) {
        ERR_PRINT(String("Passed areaType is too large. {0} (of max allowed {1}).").format(Array::make(area_type, UCHAR_MAX)));
        return -1;
    }
    if (_input_geometry->getConvexVolumeCount() >= (DetourInputGeometry::MAX_VOLUMES - 1)) {
        ERR_PRINT("Cannot mark any more convex area, limit reached.");
        return -1;
    }

    float *vert_array = new float[vertices.size() * 3];
    float miny = 10000000.0f;
    for (int i = 0; i < vertices.size(); ++i) {
        Vector3 vertex = vertices[i];
        vert_array[i * 3 + 0] = vertex.x;
        vert_array[i * 3 + 1] = vertex.y;
        vert_array[i * 3 + 2] = vertex.z;
        if (vertex.y < miny) {
            miny = vertex.y;
        }
    }

    _input_geometry->addConvexVolume(vert_array, vertices.size(), miny, miny + height, area_type);
    delete[] vert_array;
    int id = _input_geometry->getConvexVolumeCount() - 1;
    _marked_area_ids.push_back(id);
    return id;
}

void DetourNavigation::remove_convex_area_marker(int id) {
    _input_geometry->deleteConvexVolume(id);
    for (int i = 0; i < static_cast<int>(_marked_area_ids.size()); ++i) {
        if (_marked_area_ids[i] == id) {
            _marked_area_ids.erase(_marked_area_ids.begin() + i);
            break;
        }
    }
    _removed_marked_area_ids.push_back(id);
}

int DetourNavigation::add_off_mesh_connection(Vector3 from, Vector3 to, bool bidirectional, float radius, int area_type) {
    if (_off_mesh_connections.size() >= DetourInputGeometry::MAX_OFFMESH_CONNECTIONS) {
        ERR_PRINT("Cannot add any more off-mesh connections. Limit reached.");
        return -1;
    }

    float start[3] = { from.x, from.y, from.z };
    float end[3] = { to.x, to.y, to.z };
    unsigned short flags;
    switch (area_type) {
        case POLY_AREA_GROUND:
        case POLY_AREA_ROAD:
        case POLY_AREA_GRASS:
            flags = POLY_FLAGS_WALK;
            break;
        case POLY_AREA_DOOR:
            flags = POLY_FLAGS_DOOR;
            break;
        case POLY_AREA_WATER:
            flags = POLY_FLAGS_SWIM;
            break;
        case POLY_AREA_JUMP:
            flags = POLY_AREA_JUMP;
            break;
        default:
            ERR_PRINT(String("Unable to add off-mesh connection. Unknown area type: {0}").format(Array::make(area_type)));
            return -1;
    }

    _input_geometry->addOffMeshConnection(start, end, radius, bidirectional, area_type, flags);
    int id = _input_geometry->getOffMeshConnectionCount() - 1;
    _off_mesh_connections.push_back(id);
    return id;
}

void DetourNavigation::remove_off_mesh_connection(int id) {
    _input_geometry->deleteOffMeshConnection(id);
    for (int i = 0; i < static_cast<int>(_off_mesh_connections.size()); ++i) {
        if (_off_mesh_connections[i] == id) {
            _off_mesh_connections.erase(_off_mesh_connections.begin() + i);
            break;
        }
    }
    _removed_off_mesh_connections.push_back(id);
}

bool DetourNavigation::set_query_filter(int index, String name, Dictionary weights) {
    if (index >= 16) {
        ERR_PRINT(String("Index exceeds allowed number of query filters: {0}").format(Array::make(index)));
        return false;
    }

    Array keys = weights.keys();
    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        dtCrowd *crowd = nav_mesh->get_crowd();
        dtQueryFilter *filter = crowd->getEditableFilter(index);

        for (int j = 0; j < keys.size(); ++j) {
            int area_index = keys[j];
            float weight = weights[keys[j]];
            filter->setAreaCost(area_index, weight);

            if (weight > 10000.0f) {
                switch (area_index) {
                    case POLY_AREA_WATER: filter->setExcludeFlags(filter->getExcludeFlags() ^ POLY_FLAGS_SWIM); break;
                    case POLY_AREA_JUMP: filter->setExcludeFlags(filter->getExcludeFlags() ^ POLY_FLAGS_JUMP); break;
                    case POLY_AREA_DOOR: filter->setExcludeFlags(filter->getExcludeFlags() ^ POLY_FLAGS_DOOR); break;
                    case POLY_AREA_GRASS:
                    case POLY_AREA_GROUND:
                    case POLY_AREA_ROAD: filter->setExcludeFlags(filter->getExcludeFlags() ^ POLY_FLAGS_WALK); break;
                }
            }
        }
    }

    _query_filter_indices[name] = index;
    return true;
}

Ref<DetourCrowdAgent> DetourNavigation::add_agent(const Ref<DetourCrowdAgentParameters> &parameters) {
    _navigation_mutex->lock();

    DetourNavigationMesh *nav_mesh = nullptr;
    float best_fit_factor = 10000.0f;
    for (DetourNavigationMesh *candidate : _nav_meshes) {
        float fit_factor = candidate->get_actor_fit_factor(parameters->radius, parameters->height);
        if (fit_factor > 0.0f && fit_factor < best_fit_factor) {
            best_fit_factor = fit_factor;
            nav_mesh = candidate;
        }
    }

    if (nav_mesh == nullptr) {
        ERR_PRINT(String("Unable to add agent: Too big for any crowd: radius: {0} width: {1}").format(Array::make(parameters->radius, parameters->height)));
        _navigation_mutex->unlock();
        return Ref<DetourCrowdAgent>();
    }

    if (_query_filter_indices.find(parameters->filterName) == _query_filter_indices.end()) {
        ERR_PRINT(String("Unable to add agent: Unknown filter: {0}").format(Array::make(parameters->filterName)));
        _navigation_mutex->unlock();
        return Ref<DetourCrowdAgent>();
    }

    Ref<DetourCrowdAgent> agent;
    agent.instantiate();
    agent->set_movement_mode(parameters->movementMode);
    agent->set_filter(_query_filter_indices[parameters->filterName]);
    if (!nav_mesh->add_agent(agent, parameters)) {
        ERR_PRINT("Unable to add agent.");
        _navigation_mutex->unlock();
        return Ref<DetourCrowdAgent>();
    }

    for (DetourNavigationMesh *candidate : _nav_meshes) {
        if (candidate != nav_mesh && !candidate->add_agent(agent, parameters, false)) {
            ERR_PRINT("Unable to add agent shadow.");
            _navigation_mutex->unlock();
            return Ref<DetourCrowdAgent>();
        }
    }

    _agents.push_back(agent);
    _navigation_mutex->unlock();
    return agent;
}

void DetourNavigation::remove_agent(const Ref<DetourCrowdAgent> &agent) {
    _navigation_mutex->lock();
    if (agent.is_valid()) {
        agent->destroy();
    }
    for (int i = 0; i < static_cast<int>(_agents.size()); ++i) {
        if (_agents[i] == agent) {
            _agents.erase(_agents.begin() + i);
            break;
        }
    }
    _navigation_mutex->unlock();
}

Ref<DetourObstacle> DetourNavigation::add_cylinder_obstacle(Vector3 position, float radius, float height) {
    _navigation_mutex->lock();
    Ref<DetourObstacle> obstacle;
    obstacle.instantiate();
    obstacle->initialize(OBSTACLE_TYPE_CYLINDER, position, Vector3(radius, height, 0.0f), 0.0f);
    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        nav_mesh->add_obstacle(obstacle);
    }
    _obstacles.push_back(obstacle);
    _navigation_mutex->unlock();
    return obstacle;
}

Ref<DetourObstacle> DetourNavigation::add_box_obstacle(Vector3 position, Vector3 dimensions, float rotation_rad) {
    _navigation_mutex->lock();
    Ref<DetourObstacle> obstacle;
    obstacle.instantiate();
    obstacle->initialize(OBSTACLE_TYPE_BOX, position, dimensions, rotation_rad);
    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        nav_mesh->add_obstacle(obstacle);
    }
    _obstacles.push_back(obstacle);
    _navigation_mutex->unlock();
    return obstacle;
}

MeshInstance3D *DetourNavigation::create_debug_mesh(int index, bool draw_cache_bounds) {
    _navigation_mutex->lock();
    if (index > static_cast<int>(_nav_meshes.size()) - 1) {
        ERR_PRINT(String("Index higher than number of available navMeshes: {0} {1}").format(Array::make(index, static_cast<int>(_nav_meshes.size()))));
        _navigation_mutex->unlock();
        return nullptr;
    }

    if (!_debug_drawer) {
        _debug_drawer = new GodotDetourDebugDraw();
    }

    DetourNavigationMesh *nav_mesh = _nav_meshes[index];
    _debug_drawer->clear();
    nav_mesh->create_debug_mesh(_debug_drawer, draw_cache_bounds);

    MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(_debug_drawer->getArrayMesh());
    _navigation_mutex->unlock();
    return mesh_instance;
}

bool DetourNavigation::save(String path, bool compressed) {
    if (!_initialized) {
        ERR_PRINT("DTNavSave: Unable to save navigation data. Navigation not initialized.");
        return false;
    }

    Error result = DirAccess::make_dir_recursive_absolute(path.get_base_dir());
    if (result != Error::OK && result != Error::ERR_ALREADY_EXISTS) {
        ERR_PRINT(String("DTNavSave: Error while creating navigation file path: {0} {1}").format(Array::make(path, static_cast<int>(result))));
        return false;
    }

    Ref<FileAccess> save_file = compressed ? FileAccess::open_compressed(path, FileAccess::WRITE, FileAccess::COMPRESSION_ZSTD) : FileAccess::open(path, FileAccess::WRITE);
    if (save_file.is_null()) {
        ERR_PRINT(String("DTNavSave: Error while opening navigation save file: {0} {1}").format(Array::make(path, static_cast<int>(FileAccess::get_open_error()))));
        return false;
    }

    save_file->store_16(SAVE_DATA_VERSION);
    _navigation_mutex->lock();

    if (!_input_geometry->save(save_file)) {
        _navigation_mutex->unlock();
        ERR_PRINT("DTNavSave: Unable to save input geometry.");
        return false;
    }

    save_file->store_32(_nav_meshes.size());
    for (int i = 0; i < static_cast<int>(_nav_meshes.size()); ++i) {
        if (!_nav_meshes[i]->save(save_file)) {
            _navigation_mutex->unlock();
            ERR_PRINT(String("DTNavSave: Unable to save nav mesh {0}").format(Array::make(i)));
            return false;
        }
    }

    save_file->store_32(_query_filter_indices.size());
    for (const auto &entry : _query_filter_indices) {
        int index = entry.second;
        save_file->store_pascal_string(entry.first);
        save_file->store_32(index);

        dtCrowd *crowd = _nav_meshes[0]->get_crowd();
        dtQueryFilter *filter = crowd->getEditableFilter(index);
        save_file->store_16(filter->getExcludeFlags());
        for (int i = 0; i < DT_MAX_AREAS; ++i) {
            save_file->store_float(filter->getAreaCost(i));
        }
    }

    save_file->store_32(_agents.size());
    for (int i = 0; i < static_cast<int>(_agents.size()); ++i) {
        if (!_agents[i]->save(save_file)) {
            _navigation_mutex->unlock();
            ERR_PRINT(String("DTNavSave: Unable to save nav agent {0}").format(Array::make(i)));
            return false;
        }
    }

    save_file->store_32(_obstacles.size());
    for (int i = 0; i < static_cast<int>(_obstacles.size()); ++i) {
        if (!_obstacles[i]->save(save_file)) {
            _navigation_mutex->unlock();
            ERR_PRINT(String("DTNavSave: Unable to save obstacle {0}").format(Array::make(i)));
            return false;
        }
    }

    save_file->store_32(_marked_area_ids.size());
    for (int id : _marked_area_ids) {
        save_file->store_32(id);
    }

    save_file->store_32(_off_mesh_connections.size());
    for (int id : _off_mesh_connections) {
        save_file->store_32(id);
    }

    _navigation_mutex->unlock();
    save_file->close();
    return true;
}

bool DetourNavigation::load(String path, bool compressed) {
    if (_initialized) {
        ERR_PRINT("DTNavLoad: Unable to load new navigation data. Navigation still running, please use clear().");
        return false;
    }

    Ref<FileAccess> save_file = compressed ? FileAccess::open_compressed(path, FileAccess::READ, FileAccess::COMPRESSION_ZSTD) : FileAccess::open(path, FileAccess::READ);
    if (save_file.is_null()) {
        ERR_PRINT(String("DTNavLoad: Error while opening navigation save file: {0} {1}").format(Array::make(path, static_cast<int>(FileAccess::get_open_error()))));
        return false;
    }

    int version = save_file->get_16();
    if (version != SAVE_DATA_VERSION) {
        ERR_PRINT(String("DTNavLoad: Unknown version {0}").format(Array::make(version)));
        return false;
    }

    if (!_input_geometry->load(save_file)) {
        ERR_PRINT("DTNavLoad: Unable to load input geometry.");
        return false;
    }

    int num_nav_meshes = save_file->get_32();
    for (int i = 0; i < num_nav_meshes; ++i) {
        DetourNavigationMesh *nav_mesh = memnew(DetourNavigationMesh);
        if (!nav_mesh->load(_input_geometry, _recast_context, save_file)) {
            memdelete(nav_mesh);
            ERR_PRINT("DTNavLoad: Unable to load navmesh.");
            return false;
        }
        _nav_meshes.push_back(nav_mesh);
    }

    int num_query_filters = save_file->get_32();
    for (int i = 0; i < num_query_filters; ++i) {
        String name = save_file->get_pascal_string();
        int index = save_file->get_32();
        _query_filter_indices[name] = index;

        int exclude_flags = save_file->get_16();
        float area_costs[DT_MAX_AREAS];
        for (int j = 0; j < DT_MAX_AREAS; ++j) {
            area_costs[j] = save_file->get_float();
        }

        for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
            dtCrowd *crowd = nav_mesh->get_crowd();
            dtQueryFilter *filter = crowd->getEditableFilter(index);
            filter->setExcludeFlags(exclude_flags);
            for (int j = 0; j < DT_MAX_AREAS; ++j) {
                filter->setAreaCost(j, area_costs[j]);
            }
        }
    }

    int num_agents = save_file->get_32();
    for (int i = 0; i < num_agents; ++i) {
        Ref<DetourCrowdAgent> agent;
        agent.instantiate();
        if (!agent->load(save_file)) {
            ERR_PRINT("DTNavLoad: Unable to load agent.");
            return false;
        }

        Ref<DetourCrowdAgentParameters> params;
        params.instantiate();
        if (!agent->load_parameter_values(params, save_file)) {
            ERR_PRINT("DTNavLoad: Unable to load agent parameter values.");
            return false;
        }
        agent->set_movement_mode(params->movementMode);

        for (int j = 0; j < num_nav_meshes; ++j) {
            bool is_main = j == agent->get_crowd_index();
            if (!_nav_meshes[j]->add_agent(agent, params, is_main)) {
                ERR_PRINT("DTNavLoad: Unable to add loaded agent via navmesh.");
                return false;
            }
        }
        agent->set_filter(agent->get_filter_index());
        if (agent->is_moving()) {
            agent->move_towards(agent->get_target_position());
        }
        _agents.push_back(agent);
    }

    int num_obstacles = save_file->get_32();
    for (int i = 0; i < num_obstacles; ++i) {
        Ref<DetourObstacle> obstacle;
        obstacle.instantiate();
        if (!obstacle->load(save_file)) {
            ERR_PRINT(String("DTNavLoad: Unable to load obstacle {0}").format(Array::make(i)));
            return false;
        }
        for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
            nav_mesh->add_obstacle(obstacle);
        }
        _obstacles.push_back(obstacle);
    }

    int num_marked_area_ids = save_file->get_32();
    for (int i = 0; i < num_marked_area_ids; ++i) {
        _marked_area_ids.push_back(save_file->get_32());
    }

    int num_connections = save_file->get_32();
    for (int i = 0; i < num_connections; ++i) {
        _off_mesh_connections.push_back(save_file->get_32());
    }

    _stop_thread = false;
    _navigation_thread = new std::thread(&DetourNavigation::navigation_thread_function, this);
    _initialized = true;
    return true;
}

void DetourNavigation::clear() {
    _stop_thread = true;
    if (_navigation_thread) {
        if (_navigation_thread->joinable()) {
            _navigation_thread->join();
        }
        delete _navigation_thread;
    }
    _navigation_thread = nullptr;

    for (const Ref<DetourCrowdAgent> &agent : _agents) {
        agent->destroy();
    }
    _agents.clear();

    for (const Ref<DetourObstacle> &obstacle : _obstacles) {
        obstacle->destroy();
    }
    _obstacles.clear();

    std::vector<int> old_marked_areas = _marked_area_ids;
    for (int id : old_marked_areas) {
        remove_convex_area_marker(id);
    }
    _marked_area_ids.clear();

    _input_geometry->clearData();

    for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
        memdelete(nav_mesh);
    }
    _nav_meshes.clear();

    _query_filter_indices.clear();
    _initialized = false;
}

Array DetourNavigation::get_agents() {
    Array result;
    for (const Ref<DetourCrowdAgent> &agent : _agents) {
        result.append(agent);
    }
    return result;
}

Array DetourNavigation::get_obstacles() {
    Array result;
    for (const Ref<DetourObstacle> &obstacle : _obstacles) {
        if (!obstacle->is_destroyed()) {
            result.append(obstacle);
        }
    }
    return result;
}

Array DetourNavigation::get_marked_area_ids() {
    Array result;
    for (int id : _marked_area_ids) {
        result.append(id);
    }
    return result;
}

void DetourNavigation::navigation_thread_function() {
    UtilityFunctions::print("DTNav: Navigation thread started");
    double last_execution_time = 0.0;
    double seconds_to_sleep_per_frame = 1.0 / _ticks_per_second;
    int64_t milliseconds_to_sleep = 0;
    auto start = std::chrono::system_clock::now();

    while (!_stop_thread) {
        milliseconds_to_sleep = static_cast<int64_t>((seconds_to_sleep_per_frame - last_execution_time) * 1000.0 + 0.5);
        if (milliseconds_to_sleep > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_to_sleep));
        }

        start = std::chrono::system_clock::now();
        _navigation_mutex->lock();

        for (int i = 0; i < static_cast<int>(_obstacles.size()); ++i) {
            if (_obstacles[i]->is_destroyed()) {
                _obstacles.erase(_obstacles.begin() + i);
                i--;
            }
        }

        for (const Ref<DetourCrowdAgent> &agent : _agents) {
            agent->prepare_for_tick();
        }

        for (const Ref<DetourCrowdAgent> &agent : _agents) {
            agent->apply_new_target();
        }

        for (DetourNavigationMesh *nav_mesh : _nav_meshes) {
            nav_mesh->update(seconds_to_sleep_per_frame);
        }

        for (const Ref<DetourCrowdAgent> &agent : _agents) {
            agent->update(seconds_to_sleep_per_frame);
        }

        _navigation_mutex->unlock();

        auto time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
        last_execution_time = time_taken / 1000.0;
    }

    UtilityFunctions::print("DTNav: Navigation thread ended");
}

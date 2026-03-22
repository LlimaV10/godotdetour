#ifndef MESHDATAACCUMULATOR_H
#define MESHDATAACCUMULATOR_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

#include <vector>

class MeshDataAccumulator {
public:
    MeshDataAccumulator(godot::MeshInstance3D *meshInstance);
    MeshDataAccumulator();
    ~MeshDataAccumulator();

    const float *getVerts() const;
    int getVertCount() const;
    const int *getTris();
    int getTriCount();
    const float *getNormals();
    void clear();
    void append(const MeshDataAccumulator &other);
    void save(const godot::Ref<godot::FileAccess> &targetFile);
    bool load(const godot::Ref<godot::FileAccess> &sourceFile);

private:
    std::vector<float> _vertices;
    std::vector<int> _triangles;
    std::vector<float> _normals;
};

inline const float *MeshDataAccumulator::getVerts() const { return _vertices.data(); }
inline int MeshDataAccumulator::getVertCount() const { return static_cast<int>(_vertices.size() / 3); }
inline const int *MeshDataAccumulator::getTris() { return _triangles.data(); }
inline int MeshDataAccumulator::getTriCount() { return static_cast<int>(_triangles.size() / 3); }
inline const float *MeshDataAccumulator::getNormals() { return _normals.data(); }

#endif // MESHDATAACCUMULATOR_H

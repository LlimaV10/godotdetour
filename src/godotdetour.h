#ifndef GODOTDETOUR_H
#define GODOTDETOUR_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "detourcrowdagent.h"
#include "detournavigation.h"
#include "detournavigationmesh.h"
#include "detourobstacle.h"

void initialize_godotdetour_module(godot::ModuleInitializationLevel p_level);
void uninitialize_godotdetour_module(godot::ModuleInitializationLevel p_level);

#endif // GODOTDETOUR_H

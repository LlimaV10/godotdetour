#include "godotdetour.h"

#include <gdextension_interface.h>

using namespace godot;

void initialize_godotdetour_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    GDREGISTER_CLASS(DetourNavigationParameters);
    GDREGISTER_CLASS(DetourNavigation);
    GDREGISTER_CLASS(DetourNavigationMeshParameters);
    GDREGISTER_CLASS(DetourNavigationMesh);
    GDREGISTER_CLASS(DetourCrowdAgentParameters);
    GDREGISTER_CLASS(DetourCrowdAgent);
    GDREGISTER_CLASS(DetourObstacle);
}

void uninitialize_godotdetour_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT godotdetour_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_godotdetour_module);
    init_obj.register_terminator(uninitialize_godotdetour_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}

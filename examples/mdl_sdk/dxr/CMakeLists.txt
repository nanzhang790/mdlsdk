#*****************************************************************************
# Copyright 2019 NVIDIA Corporation. All rights reserved.
#*****************************************************************************

# name of the target and the resulting example
set(PROJECT_NAME examples-mdl_sdk-dxr)

# additional third-party dependencies
set(THIRDPARTY_SOURCES
    "../../thirdparty/d3dx12/d3dx12.h"
    "../../thirdparty/gltf/fx/gltf.h"
    "../../thirdparty/gltf/nlohmann/json.hpp"
    )

source_group("thirdparty" FILES ${THIRDPARTY_SOURCES})


# collect headers
set(PROJECT_HEADERS
    "mdl_d3d12/base_application.h"
    "mdl_d3d12/buffer.h"
    "mdl_d3d12/command_queue.h"
    "mdl_d3d12/common.h"
    "mdl_d3d12/descriptor_heap.h"
    "mdl_d3d12/gltf.h"
    "mdl_d3d12/mdl_d3d12.h"
    "mdl_d3d12/mdl_material_info.h"
    "mdl_d3d12/mdl_material.h"
    "mdl_d3d12/raytracing_pipeline.h"
    "mdl_d3d12/scene.h"
    "mdl_d3d12/shader.h"
    "mdl_d3d12/texture.h"
    "mdl_d3d12/utils.h"
    "mdl_d3d12/window.h"
    "mdl_d3d12/window_win32.h"
    "mdl_d3d12/window_image_file.h"
    "gui.h"
    "options.h"
    )

# collect sources
set(PROJECT_SOURCES
    "mdl_d3d12/base_application.cpp"
    "mdl_d3d12/buffer.cpp"
    "mdl_d3d12/command_queue.cpp"
    "mdl_d3d12/descriptor_heap.cpp"
    "mdl_d3d12/gltf.cpp"
    "mdl_d3d12/mdl_material.cpp"
    "mdl_d3d12/raytracing_pipeline.cpp"
    "mdl_d3d12/scene.cpp"
    "mdl_d3d12/shader.cpp"
    "mdl_d3d12/texture.cpp"
    "mdl_d3d12/utils.cpp"
    "mdl_d3d12/window_win32.cpp"
    "mdl_d3d12/window_image_file.cpp"
    "gui.cpp"
    "example_dxr.cpp"
    
    ${PROJECT_HEADERS}
    ${THIRDPARTY_SOURCES}
    )

# create target from template
create_from_base_preset(
    TARGET ${PROJECT_NAME} WIN32
    TYPE EXECUTABLE
    NAMESPACE mdl_sdk
    OUTPUT_NAME "dxr"
    SOURCES ${PROJECT_SOURCES}
    ADDITIONAL_INCLUDE_DIRS
        "../../thirdparty/d3dx12"
        "../../thirdparty/gltf"
)

# add dependencies
target_add_dependencies(TARGET ${PROJECT_NAME}
    DEPENDS
        d3d12
        imgui
        mdl::mdl_sdk
        mdl_sdk::shared
    )
    
# copy content
target_add_content(TARGET ${PROJECT_NAME}
    FILES
        "content/common.hlsl"
        "content/mdl_renderer_runtime.hlsl"
        "content/mdl_target_code_types.hlsl"
        "content/mdl_hit_programs.hlsl"
        "content/miss_programs.hlsl"
        "content/ray_gen_program.hlsl"
        "content/gltf/sphere/sphere.bin"
        "content/gltf/sphere/sphere.gltf"
        "content/hdri/hdrihaven_teufelsberg_inner_2k.exr"
    )

# creates a user settings file to setup the debugger (visual studio only, otherwise this is a no-op)
target_create_vs_user_settings(TARGET ${PROJECT_NAME})

# add tests if available
add_tests()

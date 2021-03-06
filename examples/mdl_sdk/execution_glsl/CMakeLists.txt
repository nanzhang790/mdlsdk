#*****************************************************************************
# Copyright 2019 NVIDIA Corporation. All rights reserved.
#*****************************************************************************

# name of the target and the resulting example
set(PROJECT_NAME examples-mdl_sdk-execution_glsl)

# collect resources
set(PROJECT_RESOURCES
    "example_execution_glsl.vert"
    "example_execution_glsl.frag"
    "noise_no_lut.glsl"
    ${GLEW_STATIC_LIBRARY_RELEASE}
    #${GLEW_SHARED_LIBRARY_RELEASE}
    #${MDL_DEPENDENCY_GLEW_LIBS}
    )

# collect sources
set(PROJECT_SOURCES
    "example_execution_glsl.cpp"
    ${PROJECT_RESOURCES}
    )

# create target from template
create_from_base_preset(
    TARGET ${PROJECT_NAME}
    TYPE EXECUTABLE
    NAMESPACE mdl_sdk
    OUTPUT_NAME "execution_glsl"
    SOURCES ${PROJECT_SOURCES}
)

# add dependencies
target_add_dependencies(TARGET ${PROJECT_NAME}
    DEPENDS
        opengl
        mdl::mdl_sdk
        mdl_sdk::shared
    )

# copy additional resources
target_copy_to_output_dir(TARGET ${PROJECT_NAME}
    RELATIVE    
        "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES       
        ${PROJECT_RESOURCES}
    )
    
# organize IDE folder
source_group("glsl" FILES ${PROJECT_RESOURCES})

# creates a user settings file to setup the debugger (visual studio only, otherwise this is a no-op)
target_create_vs_user_settings(TARGET ${PROJECT_NAME})

# add tests if available
add_tests()

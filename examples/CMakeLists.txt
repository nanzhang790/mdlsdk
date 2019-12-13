#*****************************************************************************
# Copyright 2019 NVIDIA Corporation. All rights reserved.
#*****************************************************************************

cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

set(MDL_BASE_FOLDER ${CMAKE_SOURCE_DIR} CACHE PATH "The folder that contains the targets to build and the cmake utility scripts.")
set(MDL_INCLUDE_FOLDER ${CMAKE_SOURCE_DIR}/../include CACHE PATH "The folder that interface headers of the MDL-SDK, i.e., mi/base, mi/neuray, ... directories.")
set(MDL_SRC_FOLDER ${CMAKE_SOURCE_DIR} CACHE INTERNAL "In the case of the examples, this is the prod/mdl_examples." )
set(MDL_EXAMPLES_FOLDER ${CMAKE_SOURCE_DIR} CACHE PATH "The folder that contains the mdl examples")

project(mdl-examples LANGUAGES C CXX)

#--------------------------------------------------------------------------------------------------
# configuration options
option(MDL_LOG_PLATFORM_INFOS "Prints some infos about the current build system (relevant for error reports)." ON)
option(MDL_LOG_DEPENDENCIES "Prints the list of dependencies during the generation step." OFF)
option(MDL_LOG_FILE_DEPENDENCIES "Prints the list of files that is copied after a successful build." OFF)
set(MDL_ADDITIONAL_COMPILER_DEFINES "" CACHE STRING "Additional compile defines that are passed to each of the projects")
set(MDL_ADDITIONAL_COMPILER_OPTIONS "" CACHE STRING "Additional compile options that are passed to each of the projects")

# -------------------------------------------------------------------------------------------------
# general setup
include(${MDL_BASE_FOLDER}/cmake/setup.cmake)

# presets and utility functions used in all CMakeLists
include(${MDL_BASE_FOLDER}/cmake/utilities.cmake)

#--------------------------------------------------------------------------------------------------
# list of modules in defined order
IF(WIN32) 
  set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_EXE_LINKER_FLAGS
        )
  foreach(CompilerFlag ${CompilerFlags})
    #string(REPLACE "/permissive-" " " ${CompilerFlag} "${${CompilerFlag}}")
    #set (CompilerFlag ${CompilerFlag} "/permissive+")
  endforeach()
ELSE(WIN32)
ENDIF(WIN32)

SET (CMAKE_CXX_EXTENSIONS OFF)

# third party
if(MDL_ENABLE_OPENGL_EXAMPLES OR MDL_ENABLE_D3D12_EXAMPLES)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/thirdparty/imgui)
endif()
add_subdirectory(${MDL_EXAMPLES_FOLDER}/thirdparty/tinyxml2)

# MDL SDK Examples
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/shared)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/archives)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/calls)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/compilation)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/discovery)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/distilling)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/execution_native)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/generate_mdl_identifier)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/instantiation)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/mdle)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/modules)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/start_shutdown)
add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/traversal)

if(MDL_ENABLE_OPENGL_EXAMPLES)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/distilling_glsl)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/execution_glsl)
endif()

if(MDL_ENABLE_CUDA_EXAMPLES)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/execution_cuda)
    if(MDL_ENABLE_OPENGL_EXAMPLES)
        add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/df_cuda)
    endif()
endif()

if(MDL_ENABLE_D3D12_EXAMPLES)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/dxr)
endif()

if(MDL_ENABLE_QT_EXAMPLES)
    add_subdirectory(${MDL_EXAMPLES_FOLDER}/mdl_sdk/mdl_browser)
endif()

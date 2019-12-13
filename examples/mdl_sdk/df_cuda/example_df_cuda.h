/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef EXAMPLE_DF_CUDA_H
#define EXAMPLE_DF_CUDA_H

#include <vector_types.h>
#include <texture_types.h>

struct Target_code_data;

enum Mdl_test_type {
    MDL_TEST_EVAL    = 0,  // only use BSDF evaluation
    MDL_TEST_SAMPLE  = 1,  // only use BSDF sampling
    MDL_TEST_MIS     = 2,  // multiple importance sampling
    MDL_TEST_MIS_PDF = 3,  // multiple importance sampling, but use BSDF explicit pdf computation
    MDL_TEST_NO_ENV  = 4,  // no environment sampling
    MDL_TEST_COUNT
};

struct Env_accel {
    unsigned int alias;
    float q;
    float pdf;
};

namespace
{
    #if defined(__CUDA_ARCH__)
    __host__ __device__
    #endif
    inline uint2 make_invalid()
    {
        uint2 index_pair;
        index_pair.x = ~0;
        index_pair.y = ~0;
        return index_pair;
    }
}

struct Df_cuda_material
{
    #if defined(__CUDA_ARCH__)
    __host__ __device__
    #endif
    Df_cuda_material()
        : compiled_material_index(0)
        , argument_block_index(~0)
        , bsdf(make_invalid())
        , edf(make_invalid())
        , emission_intensity(make_invalid())
        , volume_absorption(make_invalid())
        , thin_walled(make_invalid())
    {
    }

    // used on host side only
    unsigned int compiled_material_index;

    // the argument block index of this material (~0 if not used)
    unsigned int argument_block_index;

    // pair of target_code_index and function_index to identify the bsdf
    uint2 bsdf;

    // pair of target_code_index and function_index to identify the edf
    uint2 edf;

    // pair of target_code_index and function_index for intensity
    uint2 emission_intensity;

    // pair of target_code_index and function_index for volume absorption
    uint2 volume_absorption;

    // pair of target_code_index and function_index for thin_walled
    uint2 thin_walled;
};


struct Kernel_params {
    // display
    uint2         resolution;
    float         exposure_scale;
    unsigned int *display_buffer;
    float3       *accum_buffer;

    // parameters
    unsigned int iteration_start;
    unsigned int iteration_num;
    unsigned int mdl_test_type;
    unsigned int max_path_length;
    unsigned int use_derivatives;
    unsigned int disable_aa;

    // camera
    float3 cam_pos;
    float3 cam_dir;
    float3 cam_right;
    float3 cam_up;
    float  cam_focal;

    // environment
    uint2                env_size;
    cudaTextureObject_t  env_tex;
    Env_accel           *env_accel;

    // point light
    float3 light_pos;
    float3 light_intensity;

    // material data
    Target_code_data   *tc_data;
    char const        **arg_block_list;
    unsigned int        current_material;
    Df_cuda_material   *material_buffer;
};

#endif // EXAMPLE_DF_CUDA_H

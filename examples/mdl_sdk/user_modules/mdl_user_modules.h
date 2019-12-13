/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/

#ifndef MDL_USER_MODULES_H
#define MDL_USER_MODULES_H

// CUDA vector types
#include <vector_types.h>

struct __align__(2) bool2
{
    bool x, y;
};

struct bool3
{
    bool x, y, z;
};

struct __align__(4) bool4
{
    bool x, y, z, w;
};

typedef char const *string;

typedef bool vbool2 __attribute((ext_vector_type(2)));
typedef bool vbool3 __attribute((ext_vector_type(3)));
typedef bool vbool4 __attribute((ext_vector_type(4)));

typedef int vint2 __attribute((ext_vector_type(2)));
typedef int vint3 __attribute((ext_vector_type(3)));
typedef int vint4 __attribute((ext_vector_type(4)));

typedef float vfloat2 __attribute((ext_vector_type(2)));
typedef float vfloat3 __attribute((ext_vector_type(3)));
typedef float vfloat4 __attribute((ext_vector_type(4)));
typedef float vfloat3x3 __attribute((ext_vector_type(9)));
typedef float vfloat4x4 __attribute((ext_vector_type(16)));

typedef double vdouble2 __attribute((ext_vector_type(2)));
typedef double vdouble3 __attribute((ext_vector_type(3)));
typedef double vdouble4 __attribute((ext_vector_type(4)));
typedef double vdouble3x3 __attribute((ext_vector_type(9)));
typedef double vdouble4x4 __attribute((ext_vector_type(16)));


#include "mdl_runtime.h"


// Structs used by the MDL SDK API.
struct State_core;
struct State_environment;
struct Exception_state;

//
// Some helper functions
//

// Convert a scalar bool2 to a vector bool2.
static vbool2 make_vector(bool2 const &v)
{
    return vbool2{ v.x, v.y };
}

// Convert a scalar bool3 to a vector bool3.
static vbool3 make_vector(bool3 const &v)
{
    return vbool3{ v.x, v.y, v.z };
}

// Convert a scalar bool4 to a vector bool4.
static vbool4 make_vector(bool4 const &v)
{
    return vbool4{ v.x, v.y, v.z, v.w };
}

// Convert a scalar int2 to a vector int2.
static vint2 make_vector(int2 const &v)
{
    return vint2{ v.x, v.y };
}

// Convert a scalar int3 to a vector int3.
static vint3 make_vector(int3 const &v)
{
    return vint3{ v.x, v.y, v.z };
}

// Convert a scalar int4 to a vector int4.
static vint4 make_vector(int4 const &v)
{
    return vint4{ v.x, v.y, v.z, v.w };
}

// Convert a scalar float2 to a vector float2.
static vfloat2 make_vector(float2 const &v)
{
    return vfloat2{ v.x, v.y };
}

// Convert a scalar float3 to a vector float3.
static vfloat3 make_vector(float3 const &v)
{
    return vfloat3{ v.x, v.y, v.z };
}

// Convert a scalar float4 to a vector float4.
static vfloat4 make_vector(float4 const &v)
{
    return vfloat4{ v.x, v.y, v.z, v.w };
}

// Convert a scalar double2 to a vector double2.
static vdouble2 make_vector(double2 const &v)
{
    return vdouble2{ v.x, v.y };
}

// Convert a scalar double3 to a vector double3.
static vdouble3 make_vector(double3 const &v)
{
    return vdouble3{ v.x, v.y, v.z };
}

// Convert a scalar double4 to a vector double4.
static vdouble4 make_vector(double4 const &v)
{
    return vdouble4{ v.x, v.y, v.z, v.w };
}


namespace state {

enum coordinate_space {
    coordinate_internal,
    coordinate_object,
    coordinate_world
};

}  // namespace state

// The internal space according to the "internal_space" backend option.
// Defaults to world space.
extern state::coordinate_space INTERNAL_SPACE;

#endif  // MDL_USER_MODULES_H

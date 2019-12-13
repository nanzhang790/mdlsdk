/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

/// \file
/// \brief Helper class for handling common MDL operations.


#ifndef MDL_SDK_EXAMPLES_MDL_BROWSER_MDL_HELPER_H
#define MDL_SDK_EXAMPLES_MDL_BROWSER_MDL_HELPER_H

#include <string>

class Mdl_helper
{
public:
    // get the simple name of module, package, material, ...
    static std::string qualified_to_simple_name(const std::string& qualified_name);
};

#endif

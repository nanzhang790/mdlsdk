#*****************************************************************************
# Copyright 2019 NVIDIA Corporation. All rights reserved.
#*****************************************************************************

# -------------------------------------------------------------------------------------------------
# script expects the following variables:
# - __TARGET_ADD_DEPENDENCY_TARGET
# - __TARGET_ADD_DEPENDENCY_DEPENDS
# - __TARGET_ADD_DEPENDENCY_COMPONENTS
# - __TARGET_ADD_DEPENDENCY_NO_RUNTIME_COPY
# - __TARGET_ADD_DEPENDENCY_NO_LINKING
# -------------------------------------------------------------------------------------------------

# add include directories and link dependencies
target_include_directories(${__TARGET_ADD_DEPENDENCY_TARGET} 
    PRIVATE
        ${MDL_INCLUDE_FOLDER}
    )

# copy runtime dependencies
# copy system libraries only on windows
if(NOT __TARGET_ADD_DEPENDENCY_NO_RUNTIME_COPY)
    if(WINDOWS)
        target_copy_to_output_dir(TARGET ${__TARGET_ADD_DEPENDENCY_TARGET}
            FILES      
                ${MDL_BASE_FOLDER}/../nt-x86-64/lib/libmdl_sdk.dll
                ${MDL_BASE_FOLDER}/../nt-x86-64/lib/dds.dll
                ${MDL_BASE_FOLDER}/../nt-x86-64/lib/nv_freeimage.dll
            )
    endif()
endif()

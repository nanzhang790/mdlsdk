/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/mdl_d3d12/mdl_d3d12.h

#ifndef MDL_D3D12_COMMON_H
#define MDL_D3D12_COMMON_H

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <D3d12.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <D3d12SDKLayers.h>
#include <wrl.h>
#include <Windows.h>

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include "utils.h"

namespace mdl_d3d12
{
    using namespace DirectX;
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    typedef ID3D12Device5 D3DDevice;
    typedef ID3D12GraphicsCommandList4 D3DCommandList;

    // Identifies on which heap and which index a resource view is located.
    // A simple integer could do as well, but this is more explicit.
    struct Descriptor_heap_handle
    {
        friend class Descriptor_heap;

        explicit Descriptor_heap_handle();
        virtual ~Descriptor_heap_handle() = default;

        bool is_valid() const { return m_descriptor_heap != 0; }
        size_t get_heap_index() const { return m_index; }

        operator size_t() const { return m_index; }

    private:
        explicit Descriptor_heap_handle(Descriptor_heap* heap, size_t index);
        Descriptor_heap* m_descriptor_heap;
        size_t m_index;
    };

}


#endif

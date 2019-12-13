/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/mdl_d3d12/descriptor_heap.h

#ifndef MDL_D3D12_DESCRIPTOR_HEAP_H
#define MDL_D3D12_DESCRIPTOR_HEAP_H

#include "common.h"
#include "base_application.h"
#include "buffer.h"

namespace mdl_d3d12
{
    class Base_application;
    class Raytracing_acceleration_structure;
    class Texture;

    class Descriptor_heap
    {
        // TODO supposed to be used to track states and improve error handling
        struct Entry
        {
        };

    public:
        explicit Descriptor_heap(Base_application* app, 
                                 D3D12_DESCRIPTOR_HEAP_TYPE type, 
                                 size_t size, 
                                 std::string debug_name);
        virtual ~Descriptor_heap();

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(size_t index)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle(m_cpu_heap_start);
            handle.ptr += index * m_element_size;
            return handle;
        }
        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(size_t index)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle(m_gpu_heap_start);
            handle.ptr += index * m_element_size;
            return handle;
        }

        Descriptor_heap_handle add_empty_view();

        Descriptor_heap_handle add_shader_resource_view(Index_buffer* buffer)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc;
            if (!buffer->get_shader_resource_view_description(desc))
                return Descriptor_heap_handle();

            return add_shader_resource_view(desc, buffer->get_resource());
        }

        template<typename T>
        Descriptor_heap_handle add_shader_resource_view(Structured_buffer<T>* buffer)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc;
            if (!buffer->get_shader_resource_view_description(desc))
                return Descriptor_heap_handle();

            return add_shader_resource_view(desc, buffer->get_resource());
        }

        Descriptor_heap_handle add_render_target_view(Texture* texture);
        bool replace_by_render_target_view(
            Texture* texture, const Descriptor_heap_handle& index);

        Descriptor_heap_handle add_shader_resource_view(Buffer* buffer, bool raw);
        bool replace_by_shader_resource_view(
            Buffer* buffer, bool raw, const Descriptor_heap_handle& index);

        Descriptor_heap_handle add_shader_resource_view(Raytracing_acceleration_structure* tlas);
        Descriptor_heap_handle add_shader_resource_view(Texture* texture);

        Descriptor_heap_handle add_unordered_access_view(Texture* texture);
        bool replace_by_unordered_access_view(
            Texture* texture, const Descriptor_heap_handle& index);

        Descriptor_heap_handle add_constant_buffer_view(const Constant_buffer_base* constants);

        bool replace_by_constant_buffer_view(
            const Constant_buffer_base* constants, const Descriptor_heap_handle& index);

        ID3D12DescriptorHeap* get_heap() { return m_heap.Get(); }

    private:

        Descriptor_heap_handle add_shader_resource_view(
            const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, 
            ID3D12Resource* resource);

        void replace_by_shader_resource_view(
            const D3D12_SHADER_RESOURCE_VIEW_DESC& desc,
            ID3D12Resource* resource,
            const Descriptor_heap_handle& index);

        Descriptor_heap_handle add_constant_buffer_view(
            const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);

        void replace_by_constant_buffer_view(
            const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc, 
            const Descriptor_heap_handle& index);

        Descriptor_heap_handle add_unordered_access_view(
            const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc,
            ID3D12Resource* resource,
            ID3D12Resource* counter_resource);

        void replace_by_unordered_access_view(
            const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc,
            ID3D12Resource* resource,
            ID3D12Resource* counter_resource, 
            const Descriptor_heap_handle& index);

        Base_application* m_app;
        const std::string m_debug_name;
        D3D12_DESCRIPTOR_HEAP_TYPE m_type;
        size_t m_size;
        size_t m_element_size;

        std::vector<Entry> m_entries;
        ComPtr<ID3D12DescriptorHeap> m_heap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_heap_start;
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_heap_start;
    };
}
#endif

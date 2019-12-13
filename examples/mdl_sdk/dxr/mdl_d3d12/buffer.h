/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/mdl_d3d12/buffer.h

#ifndef MDL_D3D12_BUFFER_H
#define MDL_D3D12_BUFFER_H

#include "common.h"
#include "base_application.h"

namespace mdl_d3d12
{
    class Base_application;

    class Buffer
    {

    public:
        explicit Buffer(Base_application* app, size_t size_in_byte, std::string debug_name);
        virtual ~Buffer() = default;

        size_t get_size_in_byte() const { return m_size_in_byte; }

        template<typename T>
        bool set_data(const T* data)
        {
            return set_data((void*) data);
        }

        bool upload(D3DCommandList* command_list);

        ID3D12Resource* get_resource() const { return m_resource.Get(); }

        bool get_shader_resource_view_description_raw(D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const
        {
            desc = {};
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = static_cast<UINT>(m_size_in_byte / 4);
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            desc.Buffer.StructureByteStride = 0;
            return true;
        }

    protected:
        bool set_data(const void* data);

        Base_application* m_app;
        const std::string m_debug_name;
        const size_t m_size_in_byte;

    private:
        ComPtr<ID3D12Resource> m_resource;
        ComPtr<ID3D12Resource> m_upload_resource;
    };

    // --------------------------------------------------------------------------------------------

    template<typename TElement>
    class Structured_buffer : public Buffer
    {
    public:
        explicit Structured_buffer(
            Base_application* app, size_t element_count, std::string debug_name)
            : Buffer(app, element_count * sizeof(TElement), debug_name)
        {
        }
        virtual ~Structured_buffer() = default;

        bool get_shader_resource_view_description(D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const
        {
            desc = {};
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.NumElements = static_cast<UINT>(m_size_in_byte / sizeof(TElement));
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            desc.Buffer.StructureByteStride = static_cast<UINT>(sizeof(TElement));
            return true;
        }

        size_t get_element_count() const { return m_size_in_byte / sizeof(TElement); }
    };

    // --------------------------------------------------------------------------------------------

    template<typename TVertex>
    class Vertex_buffer : public Structured_buffer<TVertex>
    {
    public:
        explicit Vertex_buffer(Base_application* app, size_t element_count, std::string debug_name)
            : Structured_buffer<TVertex>(app, element_count, debug_name)
        {
        }
        virtual ~Vertex_buffer() = default;

        D3D12_VERTEX_BUFFER_VIEW get_vertex_buffer_view() const
        {
            D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
            vertex_buffer_view.BufferLocation = Buffer::get_resource()->GetGPUVirtualAddress();
            vertex_buffer_view.StrideInBytes = static_cast<uint32_t>(sizeof(TVertex));
            vertex_buffer_view.SizeInBytes = static_cast<uint32_t>(Buffer::get_size_in_byte());
            return std::move(vertex_buffer_view);
        }
    };

    // --------------------------------------------------------------------------------------------

    class Index_buffer : public Buffer
    {
    public:
        explicit Index_buffer(Base_application* app, size_t element_count, std::string debug_name)
            : Buffer(app, element_count * sizeof(uint32_t), debug_name)
        {
        }

        D3D12_INDEX_BUFFER_VIEW get_index_buffer_view() const
        {
            D3D12_INDEX_BUFFER_VIEW index_buffer_view;
            index_buffer_view.BufferLocation = get_resource()->GetGPUVirtualAddress();
            index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
            index_buffer_view.SizeInBytes = static_cast<uint32_t>(Buffer::get_size_in_byte());
            return index_buffer_view;
        }

        bool get_shader_resource_view_description(D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const
        {
            desc = {};
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Buffer.NumElements = static_cast<UINT>(m_size_in_byte / sizeof(uint32_t));
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            desc.Buffer.StructureByteStride = static_cast<UINT>(sizeof(uint32_t));
            return true;
        }

        size_t get_element_count() const { return m_size_in_byte / sizeof(uint32_t); }
    };

    // --------------------------------------------------------------------------------------------

    class Constant_buffer_base
    {
    public:
        explicit Constant_buffer_base(
            Base_application* app, 
            size_t size_in_byte,
            std::string debug_name)

            : m_app(app)
            , m_mapped_data(nullptr)
            , m_debug_name(debug_name)
            , m_size_in_byte(
                round_to_power_of_two(size_in_byte,
                D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))
        {
            if (log_on_failure(m_app->get_device()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(m_size_in_byte),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_resource)),
                "Failed to create resource of " + std::to_string(m_size_in_byte) + " bytes for: " + m_debug_name, SRC))
                return;

            set_debug_name(m_resource.Get(), m_debug_name);

            // map buffer and keep it mapped
            if (log_on_failure(
                m_resource->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped_data)),
                "Failed to map buffer: " + m_debug_name, SRC))
                return;
            memset(m_mapped_data, 0, m_size_in_byte);
        }

        virtual ~Constant_buffer_base() = default;

        ID3D12Resource* get_resource() const { return m_resource.Get(); }

        D3D12_CONSTANT_BUFFER_VIEW_DESC get_constant_buffer_view_description() const
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            desc.BufferLocation = m_resource->GetGPUVirtualAddress();
            desc.SizeInBytes = static_cast<UINT>(m_size_in_byte);
            return std::move(desc);
        }

        bool get_shader_resource_view_description(
            D3D12_SHADER_RESOURCE_VIEW_DESC& desc, bool raw) const
        {
            if (!raw)
            {
                log_error("Only raw buffer views are implemented yet: " + m_debug_name, SRC);
                return false;
            }

            desc = {};
            desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = static_cast<UINT>(m_size_in_byte / 4);
            desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            desc.Buffer.StructureByteStride = 0;
            return true;
        }

        // upload constant data to the GPU
        virtual void upload() = 0;

    protected:
        Base_application* m_app;
        void* m_mapped_data;
        const std::string m_debug_name;

    private:
        size_t m_size_in_byte;
        ComPtr<ID3D12Resource> m_resource;
    };

    template<typename TConstantStruct>
    class Constant_buffer : public Constant_buffer_base
    {
    public:
        explicit Constant_buffer(Base_application* app, std::string debug_name)
            : Constant_buffer_base(app, sizeof(TConstantStruct), debug_name)
        {
            memset(&data, 0, sizeof(TConstantStruct));
        }

        virtual ~Constant_buffer() = default;

        /// constant data to be copied on update
        TConstantStruct data;

        // upload constant data to the GPU
        void upload() override
        {
            memcpy(m_mapped_data, &data, sizeof(TConstantStruct));
        }
    };
}
#endif

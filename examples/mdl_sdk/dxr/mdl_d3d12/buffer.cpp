/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#include "buffer.h"

namespace mdl_d3d12
{

    Buffer::Buffer(Base_application* app, size_t size_in_byte, std::string debug_name)
        : m_app(app)
        , m_debug_name(debug_name)
        , m_size_in_byte(size_in_byte)
    {
        if (m_size_in_byte == 0)
        {
            log_error("Size of '" + m_debug_name + "' can not be zero for.", SRC);
            return;
        }

        // Create a committed resource for uploading
        auto upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_size_in_byte);

        log_on_failure(m_app->get_device()->CreateCommittedResource(
            &upload_heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &upload_buffer_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_upload_resource)),
            "Failed to create upload resource for: " + m_debug_name, SRC);
        set_debug_name(m_upload_resource.Get(), m_debug_name + "_Upload");


        // Create a committed resource for the GPU resource in a default heap.
        log_on_failure(m_app->get_device()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_size_in_byte, D3D12_RESOURCE_FLAG_NONE),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_PPV_ARGS(&m_resource)),
            "Failed to create resource: " + m_debug_name, SRC);
        set_debug_name(m_resource.Get(), m_debug_name);
    }

    bool Buffer::set_data(const void* data)
    {
        // copy data to upload buffer
        void *mapped_data;
        if (log_on_failure(m_upload_resource->Map(0, nullptr, &mapped_data),
            "Failed to map upload buffer: " + m_debug_name, SRC))
            return false;

        memcpy(mapped_data, data, m_size_in_byte);
        m_upload_resource->Unmap(0, nullptr);
        return true;
    }


    bool Buffer::upload(D3DCommandList* command_list)
    {
        command_list->ResourceBarrier(
            1, &CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), 
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

        // copy to actual resource
        command_list->CopyResource(m_resource.Get(), m_upload_resource.Get());

        command_list->ResourceBarrier(
            1, &CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), 
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        return true;
    }
}

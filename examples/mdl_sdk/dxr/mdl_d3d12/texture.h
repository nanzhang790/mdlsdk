/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/mdl_d3d12/utils.h

#ifndef MDL_D3D12_TEXTURE_H
#define MDL_D3D12_TEXTURE_H

#include "common.h"
#include "shader.h"

namespace mdl_d3d12
{
    class Base_application;

    template<typename T>
    class Structured_buffer;

    enum class GPU_access
    {
        none = 0,
        shader_resource = 1 << 0,
        render_target = 1 << 1,
        depth_stencil_target = 1 << 2,
        unorder_access = 1 << 3
    };

    class Texture
    {
    public:
        explicit Texture(
            Base_application* app, 
            GPU_access gpu_access, 
            size_t width, 
            size_t height, 
            size_t depth, 
            DXGI_FORMAT format, 
            const std::string& debug_name);

        explicit Texture(
            Base_application* app, 
            IDXGISwapChain1* swap_chain, 
            size_t swap_chain_buffer_index, 
            const std::string& debug_name);

        virtual ~Texture() = default;

        ID3D12Resource* get_resource() { return m_resource.Get(); }
        bool get_srv_description(D3D12_SHADER_RESOURCE_VIEW_DESC& out_desc) const;
        bool get_uav_description(D3D12_UNORDERED_ACCESS_VIEW_DESC& out_desc) const;

        size_t get_width() const { return m_width; }
        size_t get_height() const { return m_height; }
        DXGI_FORMAT get_format() const { return m_format; }
        size_t get_pixel_stride() const { return m_pixel_stride_in_byte; }

        void transition_to(D3DCommandList* command_list, D3D12_RESOURCE_STATES state);

        bool resize(size_t width, size_t height);
        bool resize(size_t width, size_t height, size_t depth);

        bool upload(D3DCommandList* command_list, const uint8_t* data);
        bool download(void* data);


    private:
        bool create();

        Base_application* m_app;
        std::string m_debug_name;

        GPU_access m_gpu_access;
        size_t m_width;
        size_t m_height;
        size_t m_depth;
        DXGI_FORMAT m_format;
        size_t m_pixel_stride_in_byte;

        ComPtr<ID3D12Resource> m_resource;
        ComPtr<ID3D12Resource> m_resource_upload;
        ComPtr<ID3D12Resource> m_resource_download;
        D3D12_RESOURCE_STATES m_latest_scheduled_state;


        // only if created from swap chain
        IDXGISwapChain1* m_opt_swap_chain;
        size_t m_opt_swap_chain_buffer_index;

        D3D12_CLEAR_VALUE m_clear_value;
    };


    class Environment
    {
    public:
        struct Sample_data
        {
            unsigned int alias;
            float q;
            float pdf;
        };

        explicit Environment(Base_application* app, const std::string& file_path);
        virtual ~Environment();

        // get a descriptor table that describes the resource layout
        const Descriptor_table& get_descriptor_table() const { 
            return m_resource_descriptor_table; 
        }

        float get_integral() const { return m_integral;  }

        void transition_to(D3DCommandList* command_list, D3D12_RESOURCE_STATES state);

    private:
        bool create(const std::string& file_path);

        Base_application* m_app;
        std::string m_debug_name;

        Texture* m_texture;
        Structured_buffer<Sample_data>* m_sampling_buffer;
        float m_integral;

        Descriptor_table m_resource_descriptor_table;
    };
}

#endif
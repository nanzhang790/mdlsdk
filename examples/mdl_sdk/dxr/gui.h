/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/gui.h

#ifndef MDL_D3D12_GUI_H
#define MDL_D3D12_GUI_H

#include "mdl_d3d12/common.h"

namespace mdl_d3d12
{
    class Base_application;
    struct Update_args;
    struct Render_args;
    class Scene;
    class Scene_node;
}


class Camera_controls
{
public:
    explicit Camera_controls(mdl_d3d12::Scene_node* node = nullptr);
    virtual ~Camera_controls() = default;

    bool update(const mdl_d3d12::Update_args& args);
    void set_target(mdl_d3d12::Scene_node* node);

    float movement_speed;
    float rotation_speed;
    
private:
    bool m_left_mouse_button_held;
    bool m_middle_mouse_button_held;
    int32_t m_mouse_move_start_x;
    int32_t m_mouse_move_start_y;
    mdl_d3d12::Scene_node* m_target;
};

class Gui
{
public:
    explicit Gui(mdl_d3d12::Base_application* app);
    virtual ~Gui();

    void resize(size_t width, size_t height);
    bool update(mdl_d3d12::Scene* scene, const mdl_d3d12::Update_args& args, bool show_gui);
    void render(mdl_d3d12::D3DCommandList* command_list, const mdl_d3d12::Render_args& args);

    mdl_d3d12::Scene_node* get_selected_camera() const;

private:
    mdl_d3d12::Base_application* m_app;
    mdl_d3d12::ComPtr<ID3D12DescriptorHeap> m_ui_heap;

    std::string m_selected_material;

    std::unordered_map<std::string, mdl_d3d12::Scene_node*> m_camera_map;
    std::string m_selected_camera;
    Camera_controls m_camera_controls;

};

namespace ImGui
{
    bool SliderUint(
        const char* label, 
        uint32_t* v, 
        uint32_t v_min, 
        uint32_t v_max, 
        const char* format = "%d");
}

#endif

/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#include "gui.h"

#include "mdl_d3d12/base_application.h"
#include "mdl_d3d12/scene.h"
#include "mdl_d3d12/mdl_material.h"
#include "mdl_d3d12/mdl_material_info.h"
#include "mdl_d3d12/window_win32.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

using namespace mdl_d3d12;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ------------------------------------------------------------------------------------------------


Camera_controls::Camera_controls(mdl_d3d12::Scene_node* node)
    : movement_speed(1.0f)
    , rotation_speed(1.0f)
{
    set_target(node);
}

bool Camera_controls::update(const mdl_d3d12::Update_args & args)
{
    if (m_target == nullptr)
        return false;

    ImGuiIO &io = ImGui::GetIO();
    bool camera_changed = false;

    float delta_theta = 0.0f;
    float delta_phi = 0.0f;

    float delta_right = 0.0f;
    float delta_up = 0.0f;
    float delta_forward = 0.0f;

    if (!io.WantCaptureMouse)
    {
        // zooming (actually moving in view direction)
        if (int(io.MouseWheel))
        {
            delta_forward += int(io.MouseWheel) * 0.5f * movement_speed;
            camera_changed = true;
        }

        // rotation
        if (io.MouseDown[0] && !io.MouseDown[2])
        {
            if (!m_left_mouse_button_held)
            {
                m_left_mouse_button_held = true;
                m_mouse_move_start_x = int32_t(io.MousePos.x);
                m_mouse_move_start_y = int32_t(io.MousePos.y);
            }
            else
            {
                int32_t move_dx = int32_t(io.MousePos.x) - m_mouse_move_start_x;
                int32_t move_dy = int32_t(io.MousePos.y) - m_mouse_move_start_y;
                if (abs(move_dx) > 0 || abs(move_dy) > 0)
                {
                    m_mouse_move_start_x = int32_t(io.MousePos.x);
                    m_mouse_move_start_y = int32_t(io.MousePos.y);

                    // Update camera
                    delta_phi += float(move_dx * 0.003f * rotation_speed);
                    delta_theta += float(move_dy * 0.003f * rotation_speed);
                    camera_changed = true;
                }
            }
        }
        else
            m_left_mouse_button_held = false;

        // panning
        if (!io.MouseDown[0] && io.MouseDown[2])
        {
            if (!m_middle_mouse_button_held)
            {
                m_middle_mouse_button_held = true;
                m_mouse_move_start_x = int32_t(io.MousePos.x);
                m_mouse_move_start_y = int32_t(io.MousePos.y);
            }
            else
            {
                int32_t move_dx = int32_t(io.MousePos.x) - m_mouse_move_start_x;
                int32_t move_dy = int32_t(io.MousePos.y) - m_mouse_move_start_y;
                if (abs(move_dx) > 0 || abs(move_dy) > 0)
                {
                    m_mouse_move_start_x = int32_t(io.MousePos.x);
                    m_mouse_move_start_y = int32_t(io.MousePos.y);

                    // Update camera
                    delta_right += float(move_dx * -0.01f * movement_speed);
                    delta_up += float(move_dy * 0.01f * movement_speed);
                    camera_changed = true;
                }
            }
        }
        else
            m_middle_mouse_button_held = false;
    }

    // apply changes to the node transformation
    if (camera_changed)
    {
        auto& trafo = m_target->get_local_transformation();

        // orientation

        // get axis
        DirectX::XMVECTOR right = 
            DirectX::XMVector3Rotate({1.0f, 0.0f, 0.0f, 0.0f}, trafo.rotation);

        DirectX::XMVECTOR up = 
            DirectX::XMVector3Rotate({0.0f, 1.0f, 0.0f, 0.0f}, trafo.rotation);

        DirectX::XMVECTOR forward = 
            DirectX::XMVector3Rotate({0.0f, 0.0f, -1.0f, 0.0f}, trafo.rotation);

        // get sphere coords
        float theta = std::acosf(forward.m128_f32[1]) - mdl_d3d12::PI_OVER_2;
        float phi = std::atan2f(forward.m128_f32[2], forward.m128_f32[0]) + mdl_d3d12::PI_OVER_2;

        // apply rotation 
        theta = std::max(-mdl_d3d12::PI_OVER_2 * 0.99f, 
                         std::min(theta + delta_theta, mdl_d3d12::PI_OVER_2 * 0.99f));
        phi += delta_phi;

        trafo.rotation = DirectX::XMQuaternionRotationRollPitchYaw(-theta, -phi, 0.0f);
        trafo.rotation = DirectX::XMQuaternionNormalize(trafo.rotation);

        // apply translation
        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&trafo.translation);
        pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(right, delta_right));
        pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(up, delta_up));
        pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(forward, delta_forward));

        DirectX::XMStoreFloat3(&trafo.translation, pos);
    }

    return camera_changed;
}

void Camera_controls::set_target(mdl_d3d12::Scene_node* node)
{
    if (node && m_target == node)
        return;

    m_target = node;
    m_left_mouse_button_held = false;
    m_mouse_move_start_x = 0;
    m_mouse_move_start_y = 0;
}


// ------------------------------------------------------------------------------------------------

Gui::Gui(Base_application* app)
    : m_app(app)
    , m_selected_material("")
    , m_selected_camera("")
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    log_on_failure(
        m_app->get_device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_ui_heap)),
        "Failed to create UI Descriptor heap.", SRC);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    Window_win32* win32_window = dynamic_cast<Window_win32*>(m_app->get_window());
    if (!win32_window)
    {
        log_error("The application window is not a Win32 window."
                  "Therefore the Imgui is not supported.", SRC);
        return;
    }

    ImGui_ImplWin32_Init(win32_window->get_window_handle());
    ImGui_ImplDX12_Init(m_app->get_device(), 2, DXGI_FORMAT_R8G8B8A8_UNORM,
                        m_ui_heap->GetCPUDescriptorHandleForHeapStart(),
                        m_ui_heap->GetGPUDescriptorHandleForHeapStart());

    // Setup style
    ImGui::StyleColorsDark();
    ImGui_ImplDX12_CreateDeviceObjects();

    // hook to the message pump
    win32_window->add_message_callback(ImGui_ImplWin32_WndProcHandler);
}

Gui::~Gui()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Gui::resize(size_t width, size_t height)
{
    ImGui_ImplDX12_InvalidateDeviceObjects();
    ImGui_ImplDX12_CreateDeviceObjects();
}

bool Gui::update(mdl_d3d12::Scene* scene, const mdl_d3d12::Update_args& args, bool show_gui)
{
    bool reset_rendering = false;

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // setup camera selection if required
    if (m_camera_map.size() == 0)
    {
        m_camera_map.clear();

        scene->traverse(Scene_node::Kind::Camera, [&](Scene_node* node)
        {
            std::string name = node->get_camera()->get_name();
            if (m_camera_map.find(name) == m_camera_map.end())
            {
                if (m_selected_camera.empty())
                {
                    m_selected_camera = name;
                    m_camera_controls.set_target(node);
                }

                m_camera_map[name] = node;
                return true;
            }

            size_t n = 0;
            while (true)
            {
                std::string test = name + " (" + std::to_string(++n) + ")";
                if (m_camera_map.find(test) == m_camera_map.end())
                {
                    m_camera_map[test] = node;
                    return true;
                }
            }
        });
    }

    // setup material selection
    size_t mat_count = scene->get_material_count();
    static std::map<std::string, IMaterial*> material_map;
    if (material_map.size() != mat_count)
    {
        material_map.clear();
        for (size_t i = 0; i < mat_count; ++i)
        {
            auto mat = scene->get_material(i);
            std::string name = mat->get_name();
            if (material_map.find(name) == material_map.end())
            {
                if (m_selected_material.empty())
                    m_selected_material = name;

                material_map[name] = mat;
                continue;
            }

            size_t n = 0;
            while (true)
            {
                std::string test = name + " (" + std::to_string(++n) + ")";
                if (material_map.find(test) == material_map.end())
                {
                    material_map[test] = mat;
                    break;
                }
            }
        }
    }

    // handle camera controls
    reset_rendering |= m_camera_controls.update(args);

    // stop here when the UI should not be shown
    if (!show_gui)
        return reset_rendering;

    ImGui::Begin("Scene Settings", false, ImVec2(400, 350));

    // show camera selection combo box
    if (m_camera_map.size() > 0)
    {
        if (ImGui::BeginCombo("camera", m_selected_camera.c_str()))
        {
            for (const auto& it : m_camera_map)
            {
                bool is_selected = it.first == m_selected_camera;
                if (ImGui::Selectable(it.first.c_str(), is_selected))
                {
                    m_selected_camera = it.first;
                    m_camera_controls.set_target(it.second);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SliderFloat("movement speed", &m_camera_controls.movement_speed, 0.0f, 20.0f);
        ImGui::SliderFloat("rotation speed", &m_camera_controls.rotation_speed, 0.0f, 20.0f);
    }

    // show material selection combo box

    if (mat_count > 0)
    {
        if (ImGui::BeginCombo("material", m_selected_material.c_str()))
        {
            for (const auto& it : material_map)
            {
                bool is_selected = it.first == m_selected_material;
                if (ImGui::Selectable(it.first.c_str(), is_selected))
                {
                    m_selected_material = it.first;
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Show editable material parameters if available
        Mdl_material_info* mat_info = nullptr;
        Mdl_material* mdl_mat = nullptr;
        
        if (!m_selected_material.empty())
        {
            mdl_mat = dynamic_cast<Mdl_material*>(material_map[m_selected_material]);
            if(mdl_mat)
                mat_info = mdl_mat->get_info();
        }


        if (mat_info)
        {
            const char *group_name = nullptr;
            int id = 0;
            for (std::list<Param_info>::iterator it = mat_info->params().begin(),
                 end = mat_info->params().end(); it != end; ++it, ++id)
            {
                Param_info &param = *it;

                // Ensure unique ID even for parameters with same display names
                ImGui::PushID(id);

                // Group name changed? -> Start new group with new header
                if ((!param.group_name() != !group_name) ||
                    (param.group_name() &&
                    (!group_name || strcmp(group_name, param.group_name()) != 0)))
                {
                    ImGui::Separator();
                    if (param.group_name() != nullptr)
                        ImGui::Text("%s", param.group_name());
                    group_name = param.group_name();
                }

                // Choose proper edit control depending on the parameter kind
                switch (param.kind())
                {
                case Param_info::PK_FLOAT:
                    reset_rendering |= ImGui::SliderFloat(
                        param.display_name(),
                        &param.data<float>(),
                        param.range_min(),
                        param.range_max());
                    param.update_range<float>();
                    break;
                case Param_info::PK_FLOAT2:
                    reset_rendering |= ImGui::SliderFloat2(
                        param.display_name(),
                        &param.data<float>(),
                        param.range_min(),
                        param.range_max());
                    param.update_range<float, 2>();
                    break;
                case Param_info::PK_FLOAT3:
                    reset_rendering |= ImGui::SliderFloat3(
                        param.display_name(),
                        &param.data<float>(),
                        param.range_min(),
                        param.range_max());
                    param.update_range<float, 3>();
                    break;
                case Param_info::PK_COLOR:
                    reset_rendering |= ImGui::ColorEdit3(
                        param.display_name(),
                        &param.data<float>());
                    break;
                case Param_info::PK_BOOL:
                    reset_rendering |= ImGui::Checkbox(
                        param.display_name(),
                        &param.data<bool>());
                    break;
                case Param_info::PK_INT:
                    reset_rendering |= ImGui::SliderInt(
                        param.display_name(),
                        &param.data<int>(),
                        int(param.range_min()),
                        int(param.range_max()));
                    param.update_range<int>();
                    break;
                case Param_info::PK_ARRAY:
                {
                    ImGui::Text("%s", param.display_name());
                    ImGui::Indent(16.0f * m_app->get_options()->gui_scale);
                    char *ptr = &param.data<char>();
                    for (mi::Size i = 0, n = param.array_size(); i < n; ++i)
                    {
                        std::string idx_str = std::to_string(i);
                        switch (param.array_elem_kind())
                        {
                        case Param_info::PK_FLOAT:
                            reset_rendering |= ImGui::SliderFloat(
                                idx_str.c_str(),
                                reinterpret_cast<float *>(ptr),
                                param.range_min(),
                                param.range_max());
                            break;
                        case Param_info::PK_FLOAT2:
                            reset_rendering |= ImGui::SliderFloat2(
                                idx_str.c_str(),
                                reinterpret_cast<float *>(ptr),
                                param.range_min(),
                                param.range_max());
                            break;
                        case Param_info::PK_FLOAT3:
                            reset_rendering |= ImGui::SliderFloat3(
                                idx_str.c_str(),
                                reinterpret_cast<float *>(ptr),
                                param.range_min(),
                                param.range_max());
                            break;
                        case Param_info::PK_COLOR:
                            reset_rendering |= ImGui::ColorEdit3(
                                idx_str.c_str(),
                                reinterpret_cast<float *>(ptr));
                            break;
                        case Param_info::PK_BOOL:
                            reset_rendering |= ImGui::Checkbox(
                                param.display_name(),
                                reinterpret_cast<bool *>(ptr));
                            break;
                        case Param_info::PK_INT:
                            reset_rendering |= ImGui::SliderInt(
                                param.display_name(),
                                reinterpret_cast<int *>(ptr),
                                int(param.range_min()),
                                int(param.range_max()));
                            break;
                        }
                        ptr += param.array_pitch();
                    }
                    ImGui::Unindent(16.0f * m_app->get_options()->gui_scale);
                }
                break;
                case Param_info::PK_ENUM:
                {
                    int value = param.data<int>();
                    std::string curr_value;

                    const Enum_type_info *info = param.enum_info();
                    for (size_t i = 0, n = info->values.size(); i < n; ++i)
                    {
                        if (info->values[i].value == value)
                        {
                            curr_value = info->values[i].name;
                            break;
                        }
                    }

                    if (ImGui::BeginCombo(param.display_name(), curr_value.c_str()))
                    {
                        for (size_t i = 0, n = info->values.size(); i < n; ++i)
                        {
                            const std::string &name = info->values[i].name;
                            bool is_selected = (curr_value == name);
                            if (ImGui::Selectable(
                                info->values[i].name.c_str(), is_selected))
                            {
                                param.data<int>() = info->values[i].value;
                                reset_rendering = true;
                            }
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                break;
                /*case Param_info::PK_STRING:
                    {
                        std::vector<char> buf;

                        size_t max_len = constant_table.get_max_length();
                        max_len = max_len > 63 ? max_len + 1 : 64;

                        buf.resize(max_len);

                        // fill the current value
                        unsigned curr_index = param.data<unsigned>();
                        const char *opt = constant_table.get_string(curr_index);
                        strcpy(buf.data(), opt != nullptr ? opt : "");

                        if (ImGui::InputText(
                            param.display_name(),
                            buf.data(), buf.size(),
                            ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            unsigned id = constant_table.get_id_for_string(buf.data());

                            param.data<unsigned>() = id;
                            changed = true;
                        }
                    }
                    break;
                */
                case Param_info::PK_TEXTURE:
                    ImGui::Text("%s: %d", param.display_name(), param.data<int>());
                    break;
                case Param_info::PK_LIGHT_PROFILE:
                    break;
                case Param_info::PK_BSDF_MEASUREMENT:
                    break;
                case Param_info::PK_UNKNOWN:
                default:
                    break;
                }

                ImGui::PopID();
            }

            // If any material argument changed, update the target argument block on the device
            if (reset_rendering)
            {
                mdl_mat->update_material_parameters();
            }
        }
    }

    ImGui::End();
    return reset_rendering;
}

void Gui::render(D3DCommandList* command_list, const Render_args& args)
{
    // #IMGUI Rendering the dialog
    std::vector<ID3D12DescriptorHeap*> heaps = {m_ui_heap.Get()};
    command_list->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list);
}

mdl_d3d12::Scene_node* Gui::get_selected_camera() const
{
    if(m_selected_camera.empty())
        return nullptr;
    
    const auto& it = m_camera_map.find(m_selected_camera);
    return it == m_camera_map.end() ? nullptr : it->second;
}

// ------------------------------------------------------------------------------------------------

bool ImGui::SliderUint(
    const char* label, 
    uint32_t* v, 
    uint32_t v_min, 
    uint32_t v_max, 
    const char* format)
{
    return SliderScalar(label, ImGuiDataType_U32, v, &v_min, &v_max, format);
}

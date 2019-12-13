/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/mdl_sdk/dxr/options.h

#ifndef MDL_D3D12_OPTIONS_H
#define MDL_D3D12_OPTIONS_H

#include "mdl_d3d12/common.h"
#include "mdl_d3d12/base_application.h"
#include <example_shared.h>

namespace mdl_d3d12
{
    class Example_dxr_options : public Base_options
    {
    public:
        explicit Example_dxr_options()
            : Base_options()
            , scene(get_executable_folder() + "/content/gltf/sphere/sphere.gltf")
            , point_light_enabled(false)
            , point_light_position{0.0f, 0.0f, 0.0f}
            , point_light_intensity{0.0f, 0.0f, 0.0f}
            , hdr_scale(1.0f)
            , firefly_clamp(true)
        {
            user_options["environment"] = 
                get_executable_folder() + "/content/hdri/hdrihaven_teufelsberg_inner_2k.exr";

            user_options["override_material"] = "";
        }

        std::string scene;

        bool point_light_enabled;
        DirectX::XMFLOAT3 point_light_position;
        DirectX::XMFLOAT3 point_light_intensity;

        float hdr_scale;
        bool firefly_clamp;
    };

    inline void print_options()
    {
        Example_dxr_options defaults;
        std::stringstream ss;

        ss << "\n"
        << "usage: " << BINARY_NAME << " [options] [<path_to_gltf_scene>]\n"

        << "-h|--help                 Print this text\n"
        
        << "-o <outputfile>           Image file to write result to (default: "
                                      << defaults.output_file << ")\n"

        << "--res <res_x> <res_y>     Resolution (default: " << defaults.window_width 
                                      << "x" << defaults.window_height << ")\n"

        << "--nogui                   Don't open interactive display\n"

        << "--gui_scale <factor>      GUI scaling factor (default: " << defaults.gui_scale << ")\n"

        << "--hide_gui                GUI is hidden by default, press SPACE to show it\n"

        << "--nocc                    Don't use class-compilation\n"

        << "--hdr <filename>          HDR environment map\n"
        << "                          (default: <scene_folder>/" << 
                                      defaults.user_options["environment"] << ")\n"

        << "--hdr_scale <factor>      Environment intensity scale factor\n"
        << "                          (default: " << defaults.hdr_scale << ")\n"

        << "--mdl_path <path>         MDL search path, can occur multiple times.\n"

        << "--max_path_length <num>   Maximum path length (up to one total internal reflection),\n"
        << "                          clamped to 2..100, default " << defaults.ray_depth << "\n"

        << "--iterations              Number of progressive iterations. In GUI-mode, this is the\n"
        << "                          iterations per frame. In NO-GUI-mode it is the total count.\n"

        << "--no_firefly_clamp        Disables firefly clamping used to suppress white pixels\n"
        << "                          because of low probability paths at early iterations.\n"

        << "-l <x> <y> <z> <r> <g> <b>      Add an isotropic point light with given coordinates\n"
        << "                                and intensity (flux) (default: none)\n"
            
        << "--mat <qualified_name>     override all materials using a qualified material name."
        << "\n";

        log_info(ss.str());
    }

    inline bool parse_options(
        Example_dxr_options& options, 
        LPSTR command_line_args, 
        int& return_code)
    {
        if (command_line_args && *command_line_args)
        {
            std::vector<std::string> argv = str_split(command_line_args, ' ');
            size_t argc = argv.size();

            for (size_t i = 0; i < argc; ++i)
            {
                const char* opt = argv[i].c_str();
                if (opt[0] == '-')
                {
                    if (strcmp(opt, "--nocc") == 0)
                    {
                        options.use_class_compilation = false;
                    }
                    else if (strcmp(opt, "--nogui") == 0)
                    {
                        options.no_gui = true;

                        // use a reasonable number of iterations in no-gui mode by default
                        if (options.iterations == 1) 
                            options.iterations = 1000;
                    }
                    else if (strcmp(opt, "--hide_gui") == 0)
                    {
                        options.hide_gui = true;
                    }
                    else if (strcmp(opt, "--gui_scale") == 0 && i < argc - 1)
                    {
                        options.gui_scale = static_cast<float>(atof(argv[++i].c_str()));
                    }
                    else if (strcmp(opt, "--res") == 0 && i < argc - 2)
                    {
                        options.window_width = std::max(atoi(argv[++i].c_str()), 64);
                        options.window_height = std::max(atoi(argv[++i].c_str()), 48);
                    }
                    else if (strcmp(opt, "--iterations") == 0 && i < argc - 1)
                    {
                        options.iterations = std::max(atoi(argv[++i].c_str()), 1);
                    }
                    else if (strcmp(opt, "-o") == 0 && i < argc - 1)
                    {
                        options.output_file = argv[++i];
                        std::replace(options.output_file.begin(), options.output_file.end(), 
                                     '\\', '/');
                    }
                    else if (strcmp(opt, "--hdr") == 0 && i < argc - 1)
                    {
                        std::string environment = argv[++i];
                        std::replace(environment.begin(), environment.end(), '\\', '/');
                        options.user_options["environment"] = environment;
                    }
                    else if (strcmp(opt, "--hdr_scale") == 0 && i < argc - 1)
                    {
                        options.hdr_scale = static_cast<float>(atof(argv[++i].c_str()));
                    }
                    else if (strcmp(opt, "--mat") == 0 && i < argc - 1)
                    {
                        options.user_options["override_material"] = argv[++i];
                    }
                    else if (strcmp(opt, "--no_firefly_clamp") == 0)
                    {
                        options.firefly_clamp = false;
                    }
                    else if (strcmp(opt, "-h") == 0 || strcmp(opt, "--help") == 0)
                    {
                        print_options();
                        return_code = EXIT_SUCCESS;
                        return false;
                    }
                    else if (strcmp(opt, "-l") == 0 && i < argc - 6)
                    {
                        options.point_light_enabled = true;
                        options.point_light_position = {
                            static_cast<float>(atof(argv[++i].c_str())),
                            static_cast<float>(atof(argv[++i].c_str())),
                            static_cast<float>(atof(argv[++i].c_str()))
                        };

                        options.point_light_intensity = {
                            static_cast<float>(atof(argv[++i].c_str())),
                            static_cast<float>(atof(argv[++i].c_str())),
                            static_cast<float>(atof(argv[++i].c_str()))
                        };
                    }
                    else if (strcmp(opt, "--max_path_length") == 0 && i < argc - 1)
                    {
                        options.ray_depth = std::max(2, std::min(atoi(argv[++i].c_str()), 100));
                    }
                    else if (strcmp(opt, "--mdl_path") == 0 && i < argc - 1)
                    {
                        options.mdl_paths.push_back(argv[++i]);
                    }
                    else
                    {
                        log_error("Unknown option: \"" + argv[i] + "\"", SRC);
                        print_options();
                        return_code = EXIT_FAILURE;
                        return false;
                    }
                }
                else
                {
                    // default argument is the GLTF scene to load
                    options.scene = argv[i];
                    std::replace(options.scene.begin(), options.scene.end(), '\\', '/');
                }
            }
        }


        std::string cwd = get_working_directory();
        log_info("Current working directory: " + cwd);

        size_t pos = options.scene.find_last_of('/');
        if (!is_absolute_path(options.scene))
        {
            std::string subfolder = options.scene.substr(0, pos);

            options.scene = cwd + "/" + options.scene;
            options.scene_directory = cwd;
            if (pos != std::string::npos)
                options.scene_directory += "/" + subfolder;
        }
        else
            options.scene_directory = options.scene.substr(0, pos);


        log_info("Scene directory: " + options.scene_directory);
        log_info("Scene: " + options.scene);

        // add scene folder to search paths
        options.mdl_paths.push_back(options.scene_directory);

        return_code = EXIT_SUCCESS;
        return true;
    }
}

#endif

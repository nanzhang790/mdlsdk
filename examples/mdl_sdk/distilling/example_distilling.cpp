/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/example_distilling.cpp
//
// Introduces the distillation of mdl materials to a fixed target model
// and showcases how to bake material paths to a texture

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <mi/mdl_sdk.h>
#include "example_shared.h"
#include "example_distilling_shared.h"

// Small struct used to store the result of a texture baking process
// of a material sub expression
struct Material_parameter
{
    typedef void (Remap_func(mi::base::IInterface*));

    mi::base::Handle<mi::IData>                 value;
    mi::base::Handle<mi::neuraylib::ICanvas>    texture;

    std::string                                 value_type;
    std::string                                 bake_path;

    Remap_func*                                 remap_func;

    Material_parameter() : remap_func(nullptr)
    {

    }

    Material_parameter(
        const std::string& value_type,
        Remap_func* func = nullptr)
        : value_type(value_type)
        , remap_func(func)
    {

    }
};

typedef std::map<std::string, Material_parameter> Material;

// Configure the mdl compiler.
void configure(
    mi::neuraylib::IMdl_compiler* compiler,
    const std::vector<std::string>& mdl_paths)
{
    // Set the search path for .mdl files.
    for(const auto& p : mdl_paths)
        check_success(compiler->add_module_path(p.c_str()) == 0);

    // Load freeimage library for canvas export
    check_success(compiler->load_plugin_library("nv_freeimage" MI_BASE_DLL_FILE_EXT) == 0);
}

// Creates an instance of the given material.
mi::neuraylib::IMaterial_instance* create_material_instance(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const std::string& material_name)
{
    // Load mdl module.
    std::string module_name = get_module_name(material_name);
    check_success(mdl_compiler->load_module(transaction, module_name.c_str(), context) >= 0);
    print_messages(context);

    // Create a material instance from the material definition 
    // with the default arguments.
    const char *prefix = (material_name.find("::") == 0) ? "mdl" : "mdl::";
    std::string material_db_name = prefix + material_name;

    mi::base::Handle<const mi::neuraylib::IMaterial_definition> material_definition(
        transaction->access<mi::neuraylib::IMaterial_definition>(
            material_db_name.c_str()));

    mi::Sint32 result;
    mi::neuraylib::IMaterial_instance* material_instance =
        material_definition->create_material_instance(0, &result);
    check_success(result == 0);

    return material_instance;
}

// Compiles the given material instance in the given compilation modes and stores
// it in the DB.
mi::neuraylib::ICompiled_material* compile_material_instance(
    const mi::neuraylib::IMaterial_instance* material_instance,
    mi::neuraylib::IMdl_execution_context* context,
    bool class_compilation)
{
    mi::Uint32 flags = class_compilation
        ? mi::neuraylib::IMaterial_instance::CLASS_COMPILATION
        : mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS;
   mi::neuraylib::ICompiled_material* compiled_material =
        material_instance->create_compiled_material(flags, context);
    check_success(print_messages(context));

    return compiled_material;
}

// Distills the given compiled material to the requested target model, 
// and returns it
const mi::neuraylib::ICompiled_material* create_distilled_material(
    mi::neuraylib::IMdl_distiller_api* distiller_api,
    const mi::neuraylib::ICompiled_material* compiled_material,
    const char* target_model)
{
    mi::Sint32 result = 0;
    mi::base::Handle<const mi::neuraylib::ICompiled_material> distilled_material(
        distiller_api->distill_material(compiled_material, target_model, nullptr, &result));
    check_success(result == 0);

    distilled_material->retain();
    return distilled_material.get();
}

// remap normal 
void remap_normal(mi::base::IInterface* icanvas)
{
    mi::base::Handle<mi::neuraylib::ICanvas> canvas(
        icanvas->get_interface<mi::neuraylib::ICanvas>());
    if(!canvas)
        return;
    // Convert normal values from the interval [-1.0,1.0] to [0.0, 1.0]
    mi::base::Handle<mi::neuraylib::ITile> tile (canvas->get_tile(0, 0));
    mi::Float32* data = static_cast<mi::Float32*>(tile->get_data());

    const mi::Uint32 n = canvas->get_resolution_x() * canvas->get_resolution_y() * 3;
    for(mi::Uint32 i=0; i<n; ++i)
    {
        data[i] = (data[i] + 1.f) * 0.5f;
    }
}

// simple roughness to glossiness conversion
void rough_to_gloss(mi::base::IInterface* ii)
{
    mi::base::Handle<mi::neuraylib::ICanvas> canvas(
        ii->get_interface<mi::neuraylib::ICanvas>());
    if(canvas)
    {
        mi::base::Handle<mi::neuraylib::ITile> tile (canvas->get_tile(0, 0));
        mi::Float32* data = static_cast<mi::Float32*>(tile->get_data());

        const mi::Uint32 n = canvas->get_resolution_x() * canvas->get_resolution_y();
        for(mi::Uint32 i=0; i<n; ++i)
        {
            data[i] = 1.0f - data[i];
        }
        return;
    }
    mi::base::Handle<mi::IFloat32> value(
        ii->get_interface<mi::IFloat32>());
    if(value)
    {
        mi::Float32 f;
        mi::get_value(value.get(), f);
        value->set_value(1.0f - f);
    }
}

// Setup material parameters according to target model and
// collect relevant bake paths
void setup_target_material(
    const std::string& target_model,
    mi::neuraylib::ITransaction* transaction,
    const mi::neuraylib::ICompiled_material* cm,
    Material& out_material)
{
   // Access surface.scattering function
    mi::base::Handle<const mi::neuraylib::IExpression_direct_call> parent_call(
        lookup_call("surface.scattering", cm));
    // ... and get its semantic
    mi::neuraylib::IFunction_definition::Semantics semantic(
        get_call_semantic(transaction, parent_call.get()));

    if(target_model == "diffuse")
    {
        // The target model is supposed to be a diffuse reflection bsdf
        check_success(semantic == 
            mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF);

        // Setup diffuse material parameters
        out_material["color"] = Material_parameter("Rgb_fp");
        out_material["roughness"] = Material_parameter("Float32");
        out_material["normal"] = Material_parameter("Float32<3>", remap_normal);

        // Specify bake paths
        out_material["color"].bake_path = "surface.scattering.tint";
        out_material["roughness"].bake_path = "surface.scattering.roughness";
        out_material["normal"].bake_path = "geometry.normal";

    }
    else if(target_model == "diffuse_glossy")
    {
        // Setup parameters for a simple diffuse - glossy material model
        out_material["diffuse_color"] = Material_parameter("Rgb_fp");
        out_material["glossy_color"]  = Material_parameter("Rgb_fp");
        out_material["glossy_roughness"]  = Material_parameter("Float32");
        out_material["glossy_weight"]  = Material_parameter("Float32");
        out_material["ior"] = Material_parameter("Float32");
        out_material["normal"] = Material_parameter("Float32<3>", remap_normal);

        // Diffuse-glossy distillation can result in a diffuse bsdf, a glossy bsdf 
        // or a fresnel weighted combination of both. Explicitly check the cases
        // and save the corresponding bake paths.

        switch(semantic)
        {
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF:
            out_material["diffuse_color"].bake_path = "surface.scattering.tint";
            break;
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_SIMPLE_GLOSSY_BSDF:
            out_material["glossy_color"].bake_path = "surface.scattering.tint";
            out_material["glossy_roughness"].bake_path =  "surface.scattering.roughness_u";
            break;
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_FRESNEL_LAYER:
            out_material["diffuse_color"].bake_path ="surface.scattering.base.tint";
            out_material["glossy_color"].bake_path =  "surface.scattering.layer.tint";
            out_material["glossy_roughness"].bake_path =  "surface.scattering.layer.roughness_u";
            out_material["glossy_weight"].bake_path =  "surface.scattering.weight";
            out_material["ior"].bake_path =  "surface.scattering.ior";
            break;
        default:
            // unknown function, nothing to bake
            break;
        }
        out_material["normal"].bake_path = "geometry.normal";
    }
    else if (target_model == "ue4" || target_model == "transmissive_pbr")
    {
        // Setup some UE4 material parameters
        out_material["base_color"] = Material_parameter("Rgb_fp");
        out_material["metallic"]  = Material_parameter("Float32");
        out_material["specular"]  = Material_parameter("Float32");
        out_material["roughness"]  = Material_parameter("Float32");
        out_material["normal"] = Material_parameter("Float32<3>", remap_normal);

        out_material["clearcoat_weight"] = Material_parameter("Float32");
        out_material["clearcoat_roughness"] = Material_parameter("Float32");
        out_material["clearcoat_normal"] = Material_parameter("Float32<3>", remap_normal);

        out_material["opacity"] = Material_parameter("Float32");

        std::string path_prefix = "surface.scattering.";

        bool is_transmissive_pbr = false;
        if (target_model == "transmissive_pbr")
        {
            is_transmissive_pbr = true;

            // insert parameters that only apply to transmissive_pbr
            out_material["anisotropy"] = Material_parameter("Float32");
            out_material["anisotropy_rotation"] = Material_parameter("Float32");
            out_material["transparency"] = Material_parameter("Float32");
            out_material["transmission_color"] = Material_parameter("Rgb_fp");

            // uniform
            out_material["attenuation_color"] = Material_parameter("Rgb_fp");
            out_material["attenuation_distance"] = Material_parameter("Float32");
            out_material["subsurface_color"] = Material_parameter("Rgb_fp");
            out_material["volume_ior"] = Material_parameter("Rgb_fp");

            // collect volume properties, they are guaranteed to exist
            out_material["attenuation_color"].bake_path = "volume.absorption_coefficient.s.v.attenuation";
            out_material["subsurface_color"].bake_path = "volume.absorption_coefficient.s.v.subsurface";
            out_material["attenuation_distance"].bake_path = "volume.scattering_coefficient.s.v.distance";
            out_material["volume_ior"].bake_path = "ior";
        }
        
        // Check for a clearcoat layer, first. If present, it is the outermost layer
        if(semantic == mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_CUSTOM_CURVE_LAYER)
        {
            // Setup clearcoat bake paths
            out_material["clearcoat_weight"].bake_path = path_prefix + "weight";
            out_material["clearcoat_roughness"].bake_path = path_prefix + "layer.roughness_u";
            out_material["clearcoat_normal"].bake_path = path_prefix + "normal";

            // Get clear-coat base layer
            parent_call = lookup_call("base", cm, parent_call.get());
            // Get clear-coat base layer semantic
            semantic = get_call_semantic(transaction, parent_call.get());
            // Extend path prefix
            path_prefix += "base.";
        }

        // Check for a weighted layer. Sole purpose of this layer is the transportation of  
        // the under-clearcoat-normal. It contains an empty base and a layer with the
        // actual material body
        if(semantic == mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_WEIGHTED_LAYER)
        {
            // Collect under-clearcoat normal
            out_material["normal"].bake_path = path_prefix + "normal";

            // Chain further
            parent_call = lookup_call("layer", cm, parent_call.get());
            semantic = get_call_semantic(transaction, parent_call.get());
            path_prefix += "layer.";
        }
        // Check for a normalized mix. This mix combines the metallic and dielectric parts
        // of the material
        if(semantic == mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_NORMALIZED_MIX)
        {
            // The top-mix component is supposed to be a glossy bsdf
            // Collect metallic weight
            out_material["metallic"].bake_path = path_prefix + "components.1.weight";

            // And other metallic parameters
            if (is_transmissive_pbr) {
                out_material["roughness"].bake_path = path_prefix + "components.1.component.roughness_u.s.r.roughness";
                out_material["anisotropy"].bake_path = path_prefix + "components.1.component.roughness_u.s.r.anisotropy";
                out_material["anisotropy_rotation"].bake_path = path_prefix + "components.1.component.roughness_u.s.r.rotation";
            }
            else
                out_material["roughness"].bake_path = path_prefix + "components.1.component.roughness_u";
            // Base_color can be taken from any of the leaf-bsdfs. It is supposed to
            // be the same.
            out_material["base_color"].bake_path = path_prefix + "components.1.component.tint";

            // Chain further
            parent_call = lookup_call(
                "components.0.component", cm, parent_call.get());
            semantic = get_call_semantic(transaction, parent_call.get());
            path_prefix += "components.0.component.";
        }
        if(semantic == mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_CUSTOM_CURVE_LAYER)
        {
            // Collect specular parameters
            out_material["specular"].bake_path = path_prefix + "weight";
            if (is_transmissive_pbr)
            {
                out_material["roughness"].bake_path = path_prefix + "layer.roughness_u.s.r.roughness";
                out_material["anisotropy"].bake_path = path_prefix + "layer.roughness_u.s.r.anisotropy";
                out_material["anisotropy_rotation"].bake_path = path_prefix + "layer.roughness_u.s.r.rotation";
            }
            else
            {
                out_material["roughness"].bake_path = path_prefix + "layer.roughness_u";
            }

            // Chain further
            parent_call = lookup_call("base", cm, parent_call.get());
            semantic = get_call_semantic(transaction, parent_call.get());
            path_prefix += "base.";
        }
        if (semantic == mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_NORMALIZED_MIX)
        {
            check_success(is_transmissive_pbr);

            out_material["transparency"].bake_path = path_prefix + "components.1.weight";
            out_material["transmission_color"].bake_path = path_prefix + "components.1.component.tint";
            // Chain further
            parent_call = lookup_call("components.0.component", cm, parent_call.get());
            semantic = get_call_semantic(transaction, parent_call.get());
            path_prefix += "components.0.component.";
        }
        if(semantic ==
            mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_MICROFACET_GGX_VCAVITIES_BSDF)
        {
            if(out_material["metallic"].bake_path.empty())
                out_material["metallic"].value = create_value(transaction, "Float32", 1.0f);
            if(out_material["roughness"].bake_path.empty())
                out_material["roughness"].bake_path = path_prefix + "roughness_u"; 
            if(out_material["base_color"].bake_path.empty())
                out_material["base_color"].bake_path = path_prefix + "tint"; 
        }
        else if(semantic ==
            mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF)
        {
            if(out_material["base_color"].bake_path.empty())
                out_material["base_color"].bake_path = path_prefix + "tint"; 
        }

        // Check for cutout-opacity
        mi::base::Handle<const mi::neuraylib::IExpression> cutout(
            cm->lookup_sub_expression("geometry.cutout_opacity"));
        if(cutout.is_valid_interface())
            out_material["opacity"].bake_path = "geometry.cutout_opacity";
    }
    else if (target_model == "specular_glossy")
    {
        // Setup parameters for the specular - glossy material model
        out_material["base_color"] = Material_parameter("Rgb_fp"); 
        out_material["f0"] = Material_parameter("Rgb_fp");
        out_material["f0_color"] = Material_parameter("Rgb_fp");
        out_material["f0_refl"] = Material_parameter("Float32");
        out_material["f0_weight"] = Material_parameter("Float32");
        out_material["glossiness"]  = Material_parameter("Float32", rough_to_gloss);
        out_material["opacity"]  = Material_parameter("Float32");
        out_material["normal_map"] = Material_parameter("Float32<3>", remap_normal);

        // Specular-glossy distillation can result in a diffuse bsdf, a glossy bsdf 
        // or a curve-weighted combination of both. Explicitly check the cases
        // and save the corresponding bake paths.
        switch(semantic)
        {
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF:
            out_material["base_color"].bake_path = "surface.scattering.tint";
            out_material["f0_weight"].value =  create_value(transaction, "Float32", 0.0f);
            out_material["f0_color"].value = create_value(transaction, "Color", mi::Color(0.0f));
            break;
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_MICROFACET_GGX_VCAVITIES_BSDF:
            out_material["f0_color"].bake_path = "surface.scattering.tint";
            out_material["f0_refl"].value = create_value(transaction, "Float32", 1.0f);
            out_material["f0_weight"].value = create_value(transaction, "Float32", 1.0f);
            out_material["glossiness"].bake_path = 
                "surface.scattering.roughness_u"; // needs inversion
            break;
        case mi::neuraylib::IFunction_definition::DS_INTRINSIC_DF_CUSTOM_CURVE_LAYER:
            out_material["base_color"].bake_path ="surface.scattering.base.tint";

            out_material["f0_color"].bake_path = "surface.scattering.layer.tint";
            out_material["f0_refl"].bake_path = "surface.scattering.normal_reflectivity";
            out_material["f0_weight"].bake_path = "surface.scattering.weight";

            out_material["glossiness"].bake_path = 
                "surface.scattering.layer.roughness_u"; // needs inversion

            break;
        default:
            // unknown function, nothing to bake
            break;
        }
        out_material["normal_map"].bake_path = "geometry.normal";
        out_material["opacity"].bake_path = "geometry.cutout_opacity";
    }
}

// Constructs a material for the target model, extracts the bake paths relevant for this
// model from the compiled material and bakes those paths into textures or constant values
void bake_target_material_inputs(
    mi::neuraylib::Baker_resource baker_resource,
    mi::Uint32 baking_samples,
    mi::neuraylib::ITransaction* transaction,
    const mi::neuraylib::ICompiled_material* cm,
    mi::neuraylib::IMdl_distiller_api* distiller_api,
    mi::neuraylib::IImage_api* image_api,
    Material& out_material)
{
    for(Material::iterator it = out_material.begin();
        it != out_material.end(); ++it)
    {
        Material_parameter& param =  it->second;

        // Do not attempt to bake empty paths
        if(param.bake_path.empty())
            continue;

        // Create baker for current path
        mi::base::Handle<const mi::neuraylib::IBaker> baker(distiller_api->create_baker(
            cm, param.bake_path.c_str(), baker_resource));
        check_success(baker.is_valid_interface());

        if(baker->is_uniform())
        {
            mi::base::Handle<mi::IData> value;
            if(param.value_type == "Rgb_fp")
            {
                mi::base::Handle<mi::IColor> v(
                    transaction->create<mi::IColor>());
                value = v->get_interface<mi::IData>();
            }
            else if(param.value_type == "Float32<3>")
            {
                mi::base::Handle<mi::IFloat32_3> v(
                    transaction->create<mi::IFloat32_3>());
                value = v->get_interface<mi::IData>();
            }
            else if(param.value_type == "Float32")
            {
                mi::base::Handle<mi::IFloat32> v(
                    transaction->create<mi::IFloat32>());
                value = v->get_interface<mi::IData>();
            }
            else
            {
                std::cout << "Ignoring unsupported value type '" << param.value_type
                    << "'" << std::endl;
                continue;
            }

            // Bake constant value
            mi::Sint32 result = baker->bake_constant(value.get());
            check_success(result == 0);

            if(param.remap_func)
                param.remap_func(value.get());

            param.value = value;
        }
        else
        {
            // Create a canvas
            mi::base::Handle<mi::neuraylib::ICanvas> canvas(
                image_api->create_canvas(param.value_type.c_str(), 1024, 1024));

            // Bake texture
            mi::Sint32 result = baker->bake_texture(canvas.get(), baking_samples);
            check_success(result == 0);

            if(param.remap_func)
                param.remap_func(canvas.get());

            param.texture = canvas;
        }
    }
}

template <typename T, typename U>
void init_value(mi::neuraylib::ICanvas* canvas, mi::IData* value, T*& out_array, U& out_value)
{
    if(canvas)
    {
        mi::base::Handle<mi::neuraylib::ITile> tile(canvas->get_tile(0, 0));
        out_array =static_cast<T*>(tile->get_data());
    }
    else if(value)
    {
        mi::get_value(value, out_value);
    }
}

void calculate_f0(mi::neuraylib::ITransaction* trans, Material& material)
{
    // if refl_weight value exists and is zero, set f0 to zero, too
    if(material["f0_weight"].value)
    {
        float v;
        mi::get_value(material["f0_weight"].value.get(), v);

        if(v==0.0f)
        {
            material["f0"].value = create_value(trans, "Color", mi::Color(0.0f));
            material["f0"].texture = 0;
            return;
        }
    }

    mi::Uint32 rx = material["f0"].texture->get_resolution_x();
    mi::Uint32 ry = material["f0"].texture->get_resolution_y();

    mi::Color   f0_color_value(0.0f);
    mi::Float32 f0_weight_value = 0.0f;
    mi::Float32 f0_refl_value = 0.0f;

    mi::Float32_3* f0 = nullptr;
    mi::Float32_3* f0_color = nullptr;
    mi::Float32* f0_weight = nullptr;
    mi::Float32* f0_refl = nullptr;

    init_value(material["f0"].texture.get(), nullptr, 
        f0, /* dummy */ f0_color_value);

    init_value(material["f0_color"].texture.get(), material["f0_color"].value.get(), 
        f0_color, f0_color_value);
    init_value(material["f0_weight"].texture.get(), material["f0_weight"].value.get(), 
        f0_weight, f0_weight_value);
    init_value(material["f0_refl"].texture.get(), material["f0_refl"].value.get(), 
        f0_refl, f0_refl_value);

    const mi::Uint32 n =rx * ry;
    for(mi::Uint32 i=0; i<n; ++i)
    {
        const mi::Float32 t = (f0_weight ? f0_weight[i] : f0_weight_value) *
            (f0_refl ? f0_refl[i] : f0_refl_value);

        f0[i][0] = (f0_color ? f0_color[i][0] : f0_color_value[0]) * t;
        f0[i][1] = (f0_color ? f0_color[i][1] : f0_color_value[1]) * t;
        f0[i][2] = (f0_color ? f0_color[i][2] : f0_color_value[2]) * t;
    }
}


// Print some information about baked material parameters to the console and
// save the baked textures to disk
void process_target_material(
    const std::string& target_model,
    const std::string& material_name,
    const Material& material,
    mi::neuraylib::IMdl_compiler* compiler)
{
    std::cout << "--------------------------------------------------------------------------------"
        << std::endl;
    std::cout << "Material model: " << target_model << std::endl;
    std::cout << "--------------------------------------------------------------------------------"
        << std::endl;

    for(Material::const_iterator it = material.begin();
        it != material.end(); ++it)
    {
        const std::string& param_name = it->first;
        const Material_parameter& param = it->second;

        std::cout << "Parameter: '" << param_name << "': ";
        if(param.bake_path.empty())
        {
            std::cout << " no matching bake path found in target material." 
                << std::endl;

            if(param.value)
                std::cout << "--> value set to ";
            if(param.texture)
                std::cout << "--> calculated ";
        }
        else 
            std::cout << "path '"<< param.bake_path << "' baked to ";

        if(param.texture)
        {
            std::cout << "texture." << std::endl << std::endl;

            // write texture to disc
            std::stringstream file_name;
            file_name << material_name << "-" << param_name << ".png";
            
            check_success(
                compiler->export_canvas(file_name.str().c_str(), param.texture.get()) == 0);
        }
        else if(param.value)
        {
            std::cout << "constant ";
            if(param.value_type == "Rgb_fp")
            {
                mi::base::Handle<mi::IColor> color(
                    param.value->get_interface<mi::IColor>());
                mi::Color c;
                color->get_value(c);
                std::cout << "color (" 
                    << c.r << ", " << c.g << ", " << c.b << ")."<< std::endl << std::endl;
            }
            else if(param.value_type == "Float32")
            {
                mi::base::Handle<mi::IFloat32> value(
                    param.value->get_interface<mi::IFloat32>());
                mi::Float32 v;
                value->get_value(v);
                std::cout << "float "  << v << "." << std::endl;
            }
            else if(param.value_type == "Float32<3>")
            {
                mi::base::Handle<mi::IFloat32_3> value(
                    param.value->get_interface<mi::IFloat32_3>());
                mi::Float32_3 v;
                value->get_value(v);
                std::cout << "vector (" 
                    << v.x << ", " << v.y << ", " << v.z << ")."<< std::endl << std::endl;
            }
        }
        std::cout 
            << "--------------------------------------------------------------------------------"
            << std::endl;
    }
}

// Prints program usage
static void usage(const char *name)
{
    std::cout
        << "usage: " << name << " [options] [<material_name1> ...]\n"
        << "-h                    print this text\n"
        << "--target              distilling target:diffuse|diffuse_glossy|ue4|transmissive_pbr|\n"
        << "                      specular_glossy (default: ue4)\n"
        << "--baker_resource      baking device: gpu|cpu|gpu_with_cpu_fallback (default: cpu)\n"
        << "--samples             baking samples (default: 4)\n"
        << "--mdl_path <path>     mdl search path, can occur multiple times.\n";

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    std::string                     target_model = "ue4";
    mi::neuraylib::Baker_resource   baker_resource = mi::neuraylib::BAKE_ON_CPU;
    mi::Uint32                      baking_samples = 4;
    std::vector<std::string>        material_names;
    std::vector<std::string>        mdl_paths = { get_samples_mdl_root() };

    // Collect command line arguments, if any
    for (int i = 1; i < argc; ++i) {
        const char *opt = argv[i];
        if (opt[0] == '-') {
            if (strcmp(opt, "--mdl_path") == 0) {
                if (i < argc - 1)
                    mdl_paths.push_back(argv[++i]);
                else
                    usage(argv[0]);
            }
            else if (strcmp(opt, "--target") == 0) {
                if (i < argc - 1)
                    target_model = argv[++i];
                else
                    usage(argv[0]);
            }
            else if (strcmp(opt, "--baker_resource") == 0) {
                if (i < argc - 1) {
                    std::string res = argv[++i];
                    if (res == "gpu")
                        baker_resource = mi::neuraylib::BAKE_ON_GPU;
                    else if (res == "gpu_with_cpu_fallback")
                        baker_resource = mi::neuraylib::BAKE_ON_GPU_WITH_CPU_FALLBACK;
                    else if (res != "cpu")
                        usage(argv[0]);
                }
                else
                    usage(argv[0]);
            }
            else if (strcmp(opt, "--samples") == 0) {
                if (i < argc - 1)
                    baking_samples = static_cast<mi::Uint32>(atoi(argv[++i]));
                else
                    usage(argv[0]);
            }
            else
                usage(argv[0]);
        }
        else
            material_names.push_back(opt);
    }
    if (material_names.empty())
        material_names.push_back(
            "::nvidia::sdk_examples::tutorials_distilling::example_distilling1");

    // Access the MDL SDK
    mi::base::Handle<mi::neuraylib::INeuray> neuray(load_and_get_ineuray());
    check_success(neuray.is_valid_interface());

    // Access the MDL SDK compiler component
    mi::base::Handle<mi::neuraylib::IMdl_compiler> mdl_compiler(
        neuray->get_api_component<mi::neuraylib::IMdl_compiler>());

    // Configure the MDL SDK 
    configure(mdl_compiler.get(), mdl_paths);

    // Start the MDL SDK
    mi::Sint32 result = neuray->start();
    check_start_success(result);
    {
        // Get MDL factory
        mi::base::Handle<mi::neuraylib::IMdl_factory> factory(
            neuray->get_api_component<mi::neuraylib::IMdl_factory>());

        // Create a transaction
        mi::base::Handle<mi::neuraylib::IDatabase> database(
            neuray->get_api_component<mi::neuraylib::IDatabase>());
        mi::base::Handle<mi::neuraylib::IScope> scope(database->get_global_scope());
        mi::base::Handle<mi::neuraylib::ITransaction> transaction(scope->create_transaction());

        for(const auto& material_name : material_names)
        {
            // Create an execution context
            mi::base::Handle<mi::neuraylib::IMdl_execution_context> context(
                factory->create_execution_context());

            // Load mdl module and create a material instance
            mi::base::Handle<mi::neuraylib::IMaterial_instance> instance(
                create_material_instance(
                    transaction.get(),
                    mdl_compiler.get(),
                    context.get(),
                    material_name));

            // Compile the material instance
            mi::base::Handle<const mi::neuraylib::ICompiled_material> compiled_material(
                compile_material_instance(instance.get(), context.get(), false));

            // Acquire distilling api used for material distilling and baking
            mi::base::Handle<mi::neuraylib::IMdl_distiller_api> distilling_api(
                neuray->get_api_component<mi::neuraylib::IMdl_distiller_api>());
            
            // Distill compiled material to diffuse/glossy material model
            mi::base::Handle<const mi::neuraylib::ICompiled_material> distilled_material(
                create_distilled_material(
                    distilling_api.get(), 
                    compiled_material.get(),
                    target_model.c_str()));

            // Acquire image api needed to create a canvas for baking
            mi::base::Handle<mi::neuraylib::IImage_api> image_api(
                neuray->get_api_component<mi::neuraylib::IImage_api>());

            Material out_material;
            // Setup result material parameters relevant for target_model
            // and collect bake paths
            setup_target_material(
                target_model,
                transaction.get(),
                distilled_material.get(), 
                out_material);

            // Bake material inputs
            bake_target_material_inputs(
                baker_resource,
                baking_samples,
                transaction.get(),
                distilled_material.get(), 
                distilling_api.get(),
                image_api.get(),
                out_material);

            if(target_model == "specular_glossy")
            {
                // the specular glossy models f0 parameter cannot
                // be directly taken from the distilling result but
                // needs to be calculated 

                // Create f0 canvas
                out_material["f0"].texture = 
                    image_api->create_canvas("Rgb_fp", 1024, 1024);

                // fill it
                calculate_f0(transaction.get(), out_material);
            }
            // Process resulting material, in this case we simply
            // print some information about the baked parameters
            // and save the textures to disk, if any
            process_target_material(
                target_model,
                get_material_name(material_name),
                out_material,
                mdl_compiler.get());
        }

        transaction->commit();
    }

    mdl_compiler = 0;

    // Shut down the MDL SDK
    check_success(neuray->shutdown() == 0);
    neuray = 0;

    // Unload the MDL SDK
    check_success(unload());

    keep_console_open();
    return EXIT_SUCCESS;
}

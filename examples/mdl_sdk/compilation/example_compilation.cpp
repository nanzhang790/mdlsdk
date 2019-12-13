/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/example_compilation.cpp
//
// Introduces compiled materials and highlights differences between different compilation modes.

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <mi/mdl_sdk.h>

#include "example_shared.h"


// Command line options structure.
struct Options {
    // Materials to use.
    std::string material_name;

    // Expression path to compile.
    std::string expr_path;

    // List of MDL module paths.
    std::vector<std::string> mdl_paths;

    // If true, changes the arguments of the instantiated material.
    // Will be set to false if the material name or expression path is changed.
    bool change_arguments;

    // The constructor.
    Options()
        : material_name("::nvidia::sdk_examples::tutorials::example_compilation")
        , expr_path("backface.scattering.tint")
        , change_arguments(true)
    {
    }
};

// Helper function to extract the module name from a fully-qualified material name.
std::string get_module_name( const std::string& material_name)
{
    size_t p = material_name.rfind( "::");
    return material_name.substr( 0, p);
}

// Utility function to dump the hash, arguments, temporaries, and fields of a compiled material.
void dump_compiled_material(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_factory* mdl_factory,
    const mi::neuraylib::ICompiled_material* cm,
    std::ostream& s)
{
    mi::base::Handle<mi::neuraylib::IValue_factory> value_factory(
        mdl_factory->create_value_factory( transaction));
    mi::base::Handle<mi::neuraylib::IExpression_factory> expression_factory(
        mdl_factory->create_expression_factory( transaction));

    mi::base::Uuid hash = cm->get_hash();
    char buffer[36];
    snprintf( buffer, sizeof( buffer),
        "%08x %08x %08x %08x", hash.m_id1, hash.m_id2, hash.m_id3, hash.m_id4);
    s << "    hash overall = " << buffer << std::endl;

    for( mi::Uint32 i = 0; i <= mi::neuraylib::SLOT_GEOMETRY_NORMAL; ++i) {
        hash = cm->get_slot_hash( mi::neuraylib::Material_slot( i));
        snprintf( buffer, sizeof( buffer),
            "%08x %08x %08x %08x", hash.m_id1, hash.m_id2, hash.m_id3, hash.m_id4);
        s << "    hash slot " << std::setw( 2) << i << " = " << buffer << std::endl;
    }

    mi::Size parameter_count = cm->get_parameter_count();
    for( mi::Size i = 0; i < parameter_count; ++i) {
        mi::base::Handle<const mi::neuraylib::IValue> argument( cm->get_argument( i));
        std::stringstream name;
        name << i;
        mi::base::Handle<const mi::IString> result(
            value_factory->dump( argument.get(), name.str().c_str(), 1));
        s << "    argument " << result->get_c_str() << std::endl;
    }

    mi::Size temporary_count = cm->get_temporary_count();
    for( mi::Size i = 0; i < temporary_count; ++i) {
        mi::base::Handle<const mi::neuraylib::IExpression> temporary( cm->get_temporary( i));
        std::stringstream name;
        name << i;
        mi::base::Handle<const mi::IString> result(
            expression_factory->dump( temporary.get(), name.str().c_str(), 1));
        s << "    temporary " << result->get_c_str() << std::endl;
    }

    mi::base::Handle<const mi::neuraylib::IExpression> body( cm->get_body());
    mi::base::Handle<const mi::IString> result( expression_factory->dump( body.get(), 0, 1));
    s << "    body " << result->get_c_str() << std::endl;

    s << std::endl;
}

// Creates an instance of "mdl::example::compilation_material".
void create_material_instance(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const char* material_name,
    const char* instance_name)
{
    // Load the "example" module.
    check_success( mdl_compiler->load_module( 
        transaction, get_module_name(material_name).c_str(), context) >= 0);
    print_messages( context);

    std::string prefix = strncmp(material_name, "::", 2) == 0 ? "mdl" : "mdl::";

    // Create a material instance from the material definition with the default arguments.
    mi::base::Handle<const mi::neuraylib::IMaterial_definition> material_definition(
        transaction->access<mi::neuraylib::IMaterial_definition>((prefix + material_name).c_str()));
    mi::Sint32 result;
    mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
        material_definition->create_material_instance( 0, &result));
    check_success( result == 0);
    transaction->store( material_instance.get(), instance_name);
}

// Compiles the given material instance in the given compilation modes, dumps the result, and stores
// it in the DB.
void compile_material_instance(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_factory* mdl_factory,
    mi::neuraylib::IMdl_execution_context* context,
    const char* instance_name,
    const char* compiled_material_name,
    bool class_compilation)
{
    mi::base::Handle<const mi::neuraylib::IMaterial_instance> material_instance(
       transaction->access<mi::neuraylib::IMaterial_instance>( instance_name));

    mi::Uint32 flags = class_compilation
        ? mi::neuraylib::IMaterial_instance::CLASS_COMPILATION
        : mi::neuraylib::IMaterial_instance::DEFAULT_OPTIONS;
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        material_instance->create_compiled_material( flags, context));
    check_success( print_messages( context));

    std::cout << "Dumping compiled material (" << ( class_compilation ? "class" : "instance")
              << " compilation) for \"" << instance_name << "\":" << std::endl << std::endl;
    dump_compiled_material( transaction, mdl_factory, compiled_material.get(), std::cout);
    std::cout << std::endl;
    transaction->store( compiled_material.get(), compiled_material_name);
}

// Changes the tint parameter of the given material instance to green.
void change_arguments(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_factory* mdl_factory,
    const char* instance_name)
{
    mi::base::Handle<mi::neuraylib::IValue_factory> value_factory(
        mdl_factory->create_value_factory( transaction));
    mi::base::Handle<mi::neuraylib::IExpression_factory> expression_factory(
        mdl_factory->create_expression_factory( transaction));

    // Edit the instance of the material definition "compilation_material".
    mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
        transaction->edit<mi::neuraylib::IMaterial_instance>( instance_name));
    check_success( material_instance.is_valid_interface());

    // Create the new argument for the "tint" parameter from scratch with the new value, and set it.
    mi::base::Handle<mi::neuraylib::IValue> tint_value(
        value_factory->create_color( 0.0f, 1.0f, 0.0f));
    mi::base::Handle<mi::neuraylib::IExpression> tint_expr(
        expression_factory->create_constant( tint_value.get()));
    check_success( material_instance->set_argument( "tint", tint_expr.get()) == 0);
}

// Generates LLVM IR target code for a subexpression of a given compiled material.
void generate_llvm_ir(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const char* compiled_material_name,
    const char* path,
    const char* fname)
{
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        transaction->edit<mi::neuraylib::ICompiled_material>( compiled_material_name));

    mi::base::Handle<mi::neuraylib::IMdl_backend> be_llvm_ir(
        mdl_compiler->get_backend( mi::neuraylib::IMdl_compiler::MB_LLVM_IR));
    check_success( be_llvm_ir->set_option( "num_texture_spaces", "16") == 0);
    check_success( be_llvm_ir->set_option( "enable_simd", "on") == 0);

    mi::base::Handle<const mi::neuraylib::ITarget_code> code_llvm_ir(
        be_llvm_ir->translate_material_expression(
            transaction, compiled_material.get(), path, fname, context));
    check_success( print_messages( context));
    check_success( code_llvm_ir);

    std::cout << "Dumping LLVM IR code for \"" << path << "\" of \"" << compiled_material_name
              << "\":" << std::endl << std::endl;
    std::cout << code_llvm_ir->get_code() << std::endl;
}

// Generates CUDA PTX target code for a subexpression of a given compiled material.
void generate_cuda_ptx(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const char* compiled_material_name,
    const char* path,
    const char* fname)
{
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        transaction->edit<mi::neuraylib::ICompiled_material>( compiled_material_name));

    mi::base::Handle<mi::neuraylib::IMdl_backend> be_cuda_ptx(
        mdl_compiler->get_backend( mi::neuraylib::IMdl_compiler::MB_CUDA_PTX));
    check_success( be_cuda_ptx->set_option( "num_texture_spaces", "16") == 0);
    check_success( be_cuda_ptx->set_option( "sm_version", "50") == 0);

    mi::base::Handle<const mi::neuraylib::ITarget_code> code_cuda_ptx(
        be_cuda_ptx->translate_material_expression(
            transaction, compiled_material.get(), path, fname, context));
    check_success( print_messages( context));
    check_success( code_cuda_ptx);

    std::cout << "Dumping CUDA PTX code for \"" << path << "\" of \"" << compiled_material_name
              << "\":" << std::endl << std::endl;
    std::cout << code_cuda_ptx->get_code() << std::endl;
}

// Generates HLSL target code for a subexpression of a given compiled material.
void generate_hlsl(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const char* compiled_material_name,
    const char* path,
    const char* fname)
{
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        transaction->edit<mi::neuraylib::ICompiled_material>( compiled_material_name));

    mi::base::Handle<mi::neuraylib::IMdl_backend> be_hlsl(
        mdl_compiler->get_backend( mi::neuraylib::IMdl_compiler::MB_HLSL));
    check_success( be_hlsl->set_option( "num_texture_spaces", "1") == 0);

    mi::base::Handle<const mi::neuraylib::ITarget_code> code_hlsl(
        be_hlsl->translate_material_expression(
            transaction, compiled_material.get(), path, fname, context));
    check_success( print_messages( context));
    check_success( code_hlsl);

    std::cout << "Dumping HLSL code for \"" << path << "\" of \"" << compiled_material_name
              << "\":" << std::endl << std::endl;
    std::cout << code_hlsl->get_code() << std::endl;
}

#ifndef MDL_SOURCE_RELEASE
// Generates GLSL target code for a subexpression of a given compiled material.
void generate_glsl(
    mi::neuraylib::ITransaction* transaction,
    mi::neuraylib::IMdl_compiler* mdl_compiler,
    mi::neuraylib::IMdl_execution_context* context,
    const char* compiled_material_name,
    const char* path,
    const char* fname)
{
    mi::base::Handle<mi::neuraylib::ICompiled_material> compiled_material(
        transaction->edit<mi::neuraylib::ICompiled_material>( compiled_material_name));

    mi::base::Handle<mi::neuraylib::IMdl_backend> be_glsl(
        mdl_compiler->get_backend( mi::neuraylib::IMdl_compiler::MB_GLSL));
    check_success( be_glsl->set_option( "glsl_version", "450") == 0);

    mi::base::Handle<const mi::neuraylib::ITarget_code> code_glsl(
        be_glsl->translate_material_expression(
            transaction, compiled_material.get(), path, fname, context));
    check_success( print_messages( context));
    check_success( code_glsl);

    std::cout << "Dumping GLSL code for \"" << path << "\" of \"" << compiled_material_name
              << "\":" << std::endl << std::endl;
    std::cout << code_glsl->get_code() << std::endl;
}
#endif // MDL_SOURCE_RELEASE

void usage( char const *prog_name)
{
    std::cout
        << "Usage: " << prog_name << " [options] [<material_name>]\n"
        << "Options:\n"
        << "  --mdl_path <path>   mdl search path, can occur multiple times.\n"
        << "  --expr_path         expression path to compile, defaults to "
           "\"backface.scattering.tint\"."
        << "  <material_name>     qualified name of materials to use, defaults to\n"
        << "                      \"::nvidia::sdk_examples::tutorials::example_compilation\"\n"
        << std::endl;
    keep_console_open();
    exit( EXIT_FAILURE);
}

int main( int argc, char* argv[])
{
    // Parse command line options
    Options options;
    options.mdl_paths.push_back(get_samples_mdl_root());

    for ( int i = 1; i < argc; ++i) {
        char const *opt = argv[i];
        if ( opt[0] == '-') {
            if ( strcmp( opt, "--mdl_path") == 0 && i < argc - 1) {
                options.mdl_paths.push_back( argv[++i]);
            } else if ( strcmp( opt, "--expr_path") == 0 && i < argc - 1) {
                options.expr_path = argv[++i];
                options.change_arguments = false;
            } else {
                std::cout << "Unknown option: \"" << opt << "\"" << std::endl;
                usage( argv[0]);
            }
        } else {
            options.material_name = opt;
            options.change_arguments = false;
        }
    }

    // Access the MDL SDK
    mi::base::Handle<mi::neuraylib::INeuray> neuray( load_and_get_ineuray());
    check_success( neuray.is_valid_interface());

    // Configure the MDL SDK
    configure( neuray.get());

    mi::base::Handle<mi::neuraylib::IMdl_compiler> mdl_compiler(
        neuray->get_api_component<mi::neuraylib::IMdl_compiler>());
    for ( std::string const &mdl_path : options.mdl_paths) {
        check_success( mdl_compiler->add_module_path( mdl_path.c_str()) == 0);
    }

    // Start the MDL SDK
    mi::Sint32 result = neuray->start();
    check_start_success( result);

    {
        mi::base::Handle<mi::neuraylib::IMdl_factory> mdl_factory(
            neuray->get_api_component<mi::neuraylib::IMdl_factory>());

        mi::base::Handle<mi::neuraylib::IDatabase> database(
            neuray->get_api_component<mi::neuraylib::IDatabase>());
        mi::base::Handle<mi::neuraylib::IScope> scope( database->get_global_scope());
        mi::base::Handle<mi::neuraylib::ITransaction> transaction( scope->create_transaction());

        {
            // Create an execution context for options and error message handling
            mi::base::Handle<mi::neuraylib::IMdl_execution_context> context(
                mdl_factory->create_execution_context());

            // Load the "example" module and create a material instance
            std::string instance_name = "instance of compilation_material";
            create_material_instance(
                transaction.get(),
                mdl_compiler.get(),
                context.get(),
                options.material_name.c_str(),
                instance_name.c_str());

            // Compile the material instance in instance compilation mode
            std::string instance_compilation_name
                = std::string( "instance compilation of ") + instance_name;
            compile_material_instance(
                transaction.get(), mdl_factory.get(), context.get(), instance_name.c_str(),
                instance_compilation_name.c_str(), false);

            // Compile the material instance in class compilation mode
            std::string class_compilation_name
                = std::string( "class compilation of ") + instance_name;
            compile_material_instance(
                transaction.get(), mdl_factory.get(), context.get(), instance_name.c_str(),
                class_compilation_name.c_str(), true);

            // Change some material argument and compile again in both modes. Note the differences
            // in instance compilation mode, whereas only the referenced parameter itself changes in
            // class compilation mode.
            if (options.change_arguments) {
                change_arguments( transaction.get(), mdl_factory.get(), instance_name.c_str());
                compile_material_instance(
                    transaction.get(), mdl_factory.get(), context.get(), instance_name.c_str(),
                    instance_compilation_name.c_str(), false);
                compile_material_instance(
                    transaction.get(), mdl_factory.get(), context.get(), instance_name.c_str(),
                    class_compilation_name.c_str(), true);
            }

            // Use the various backends to generate target code for some material expression.
            generate_llvm_ir(
                transaction.get(), mdl_compiler.get(), context.get(),
                instance_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
            generate_llvm_ir(
                transaction.get(), mdl_compiler.get(), context.get(),
                class_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
            generate_cuda_ptx(
                transaction.get(), mdl_compiler.get(), context.get(),
                instance_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
            generate_cuda_ptx(
                transaction.get(), mdl_compiler.get(), context.get(),
                class_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
#ifndef MDL_SOURCE_RELEASE
            generate_glsl(
                transaction.get(), mdl_compiler.get(), context.get(),
                instance_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
            generate_glsl(
                transaction.get(), mdl_compiler.get(), context.get(),
                class_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
#endif /*MDL_SOURCE_RELEASE*/
            generate_hlsl(
                transaction.get(), mdl_compiler.get(), context.get(),
                instance_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
            generate_hlsl(
                transaction.get(), mdl_compiler.get(), context.get(),
                class_compilation_name.c_str(),
                options.expr_path.c_str(), "tint");
        }

        transaction->commit();
    }

    // Free MDL compiler before shutting down MDL SDK
    mdl_compiler = 0;

    // Shut down the MDL SDK
    check_success( neuray->shutdown() == 0);
    neuray = 0;

    // Unload the MDL SDK
    check_success( unload());

    keep_console_open();
    return EXIT_SUCCESS;
}

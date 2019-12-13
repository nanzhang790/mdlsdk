/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// examples/example_instantiation.cpp
//
// Instantiates a material and a function definition and changes argument values.

#include <iostream>
#include <string>

#include <mi/mdl_sdk.h>

#include "example_shared.h"

const char* material_definition_name = "mdl::nvidia::sdk_examples::tutorials::example_material";
const char* material_instance_name   = "instance of example_material";
const char* function_definition_name = 
    "mdl::nvidia::sdk_examples::tutorials::example_function(color,float)";
const char* function_call_name       = "call of example_function";

// Utility function to dump the arguments of a material instance or function call.
template <class T>
void dump_instance(
    mi::neuraylib::IExpression_factory* expression_factory, const T* material, std::ostream& s)
{
    mi::Size count = material->get_parameter_count();
    mi::base::Handle<const mi::neuraylib::IExpression_list> arguments( material->get_arguments());

    for( mi::Size index = 0; index < count; index++) {

        mi::base::Handle<const mi::neuraylib::IExpression> argument(
            arguments->get_expression( index));
        std::string name = material->get_parameter_name( index);
        mi::base::Handle<const mi::IString> argument_text(
            expression_factory->dump( argument.get(), name.c_str(), 1));
        s << "    argument " << argument_text->get_c_str() << std::endl;

    }
    s << std::endl;
}

// Instantiates a material definition and a function definition.
void instantiate_definitions(
    mi::neuraylib::INeuray* neuray, mi::neuraylib::ITransaction* transaction)
{
    mi::base::Handle<mi::neuraylib::IMdl_compiler> mdl_compiler(
        neuray->get_api_component<mi::neuraylib::IMdl_compiler>());

    mi::base::Handle<mi::neuraylib::IMdl_factory> mdl_factory(
        neuray->get_api_component<mi::neuraylib::IMdl_factory>());
    mi::base::Handle<mi::neuraylib::IValue_factory> value_factory(
        mdl_factory->create_value_factory( transaction));
    mi::base::Handle<mi::neuraylib::IExpression_factory> expression_factory(
        mdl_factory->create_expression_factory( transaction));

    mi::base::Handle<mi::neuraylib::IMdl_execution_context> context(
        mdl_factory->create_execution_context());

    // Load the module "example" and access it from the DB.
    check_success( mdl_compiler->load_module(
        transaction, "::nvidia::sdk_examples::tutorials", context.get()) >= 0);
    print_messages( context.get());

    mi::base::Handle<const mi::neuraylib::IModule> module(
        transaction->access<mi::neuraylib::IModule>( "mdl::nvidia::sdk_examples::tutorials"));
    check_success( module.is_valid_interface());

    // Instantiation of a material definition
    {
        // Access the material definition "example_material".
        mi::base::Handle<const mi::neuraylib::IMaterial_definition> material_definition(
            transaction->access<mi::neuraylib::IMaterial_definition>(
                material_definition_name));

        // Since all parameter of this material definition have defaults we can instantiate the
        // definition without the need to explicitly provide initial arguments.
        mi::Sint32 result = 0;
        mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
            material_definition->create_material_instance( 0, &result));
        check_success( result == 0);

        std::cout << "Dumping material instance \"" << material_instance_name << "\":" << std::endl;
        dump_instance( expression_factory.get(), material_instance.get(), std::cout);

        // We are going to change the arguments later, so store the instance for now in the DB.
        transaction->store( material_instance.get(), material_instance_name);
    }

    // Instantiation of a function definition
    {
        // Access the function definition "example_function(color,float)".
        mi::base::Handle<const mi::neuraylib::IFunction_definition> function_definition(
            transaction->access<mi::neuraylib::IFunction_definition>(
                function_definition_name));

        // Since not all parameters of this function definition have defaults we have to provide
        // explicitly initial arguments to create an instance of the definition.
        mi::base::Handle<mi::neuraylib::IExpression_list> arguments(
            expression_factory->create_expression_list());
        mi::base::Handle<mi::neuraylib::IValue> tint_value(
            value_factory->create_color( 1.0f, 0.0f, 0.0f));
        mi::base::Handle<mi::neuraylib::IExpression> tint_expr(
            expression_factory->create_constant( tint_value.get()));
        arguments->add_expression( "tint", tint_expr.get());
        mi::base::Handle<mi::neuraylib::IValue> distance_value(
            value_factory->create_float( 2.0f));
        mi::base::Handle<mi::neuraylib::IExpression> distance_expr(
            expression_factory->create_constant( distance_value.get()));
        arguments->add_expression( "distance", distance_expr.get());

        // Instantiate the function definition using "arguments" as initial arguments.
        mi::Sint32 result = 0;
        mi::base::Handle<mi::neuraylib::IFunction_call> function_call(
            function_definition->create_function_call( arguments.get(), &result));
        check_success( result == 0);

        std::cout << "Dumping function call \"" << function_call_name << "\":" << std::endl;
        dump_instance( expression_factory.get(), function_call.get(), std::cout);

        // We are going to change the arguments later, so store the call for now in the DB.
        transaction->store( function_call.get(), function_call_name);
    }
}

// Changes the arguments of a previously create material instance or function call.
void change_arguments( mi::neuraylib::INeuray* neuray, mi::neuraylib::ITransaction* transaction)
{
    mi::base::Handle<mi::neuraylib::IMdl_factory> mdl_factory(
        neuray->get_api_component<mi::neuraylib::IMdl_factory>());
    mi::base::Handle<mi::neuraylib::IValue_factory> value_factory(
        mdl_factory->create_value_factory( transaction));
    mi::base::Handle<mi::neuraylib::IExpression_factory> expression_factory(
        mdl_factory->create_expression_factory( transaction));

    // Changing arguments: cloning the old value and modifying the clone
    {
        // Edit the instance of the material definition "example_material".
        mi::base::Handle<mi::neuraylib::IMaterial_instance> material_instance(
            transaction->edit<mi::neuraylib::IMaterial_instance>( material_instance_name));
        check_success( material_instance.is_valid_interface());

        // Get the old argument for the "tint" parameter and clone it.
        mi::base::Handle<const mi::neuraylib::IExpression_list> arguments(
            material_instance->get_arguments());
        mi::base::Handle<const mi::neuraylib::IExpression> argument(
            arguments->get_expression( "tint"));

        // Change the value of the clone and set it as new argument. Here we use prior knowledge
        // that the type of the parameter is a color and the expression is a constant.
        mi::base::Handle<mi::neuraylib::IExpression> new_argument(
            expression_factory->clone( argument.get()));
        mi::base::Handle<mi::neuraylib::IExpression_constant> new_argument_constant(
            new_argument->get_interface<mi::neuraylib::IExpression_constant>());
        mi::base::Handle<mi::neuraylib::IValue_color> new_argument_value(
            new_argument_constant->get_value<mi::neuraylib::IValue_color>());
        set_value( new_argument_value.get(), mi::math::Color( 0.0f, 1.0f, 0.0f));
        check_success( material_instance->set_argument( "tint", new_argument.get()) == 0);

        std::cout << "Dumping modified material instance \"" << material_instance_name << "\":"
            << std::endl;
        dump_instance( expression_factory.get(), material_instance.get(), std::cout);
    }

    // Changing arguments: using the argument editor
    {
        {
            // Edit the instance of the function definition "volume_coefficient()". Here we use
            // prior knowledge that the type of the parameter is a color.
            mi::neuraylib::Argument_editor argument_editor(
                transaction, function_call_name, mdl_factory.get());
            mi::math::Color blue( 0.0f, 0.0f, 1.0f);
            check_success( argument_editor.set_value( "tint", blue) == 0);
        }

        mi::base::Handle<mi::neuraylib::IFunction_call> function_call(
            transaction->edit<mi::neuraylib::IFunction_call>( function_call_name));
        check_success( function_call.is_valid_interface());

        std::cout << "Dumping modified function call \"" << function_call_name << "\":"
            << std::endl;
        dump_instance( expression_factory.get(), function_call.get(), std::cout);
    }
}

// Iterates over an annotation block and prints annotations and their parameters.
void print_annotations( const mi::neuraylib::IAnnotation_block* anno_block)
{
    mi::neuraylib::Annotation_wrapper annotations( anno_block);
    std::cout << "There are " 
        << annotations.get_annotation_count() << " annotation(s).\n";

    for( mi::Size a = 0; a < annotations.get_annotation_count(); ++a)
    {
        std::string signature( annotations.get_annotation_name( a));

        // strip signature
        size_t p = signature.find( '(');
        std::string name;
        if( p != std::string::npos)
            name = signature.substr( 0, p);

        p = name.find_last_of( ':');
        if( p != std::string::npos)
            name = name.substr( p + 1);

        std::cout << " '" << name << "' with " 
            << annotations.get_annotation_param_count( a) << " parameter(s):\n";
        for( mi::Size p = 0; p < annotations.get_annotation_param_count( a); ++p)
        {
            mi::base::Handle<const mi::neuraylib::IType> type_handle(
                annotations.get_annotation_param_type( a, p));

            std::cout << "  '" << annotations.get_annotation_param_name( a, p)
                << "' of kind of type '"
                << type_handle->get_kind() << "' -> ";

            switch( type_handle->get_kind())
            {
                case mi::neuraylib::IType::TK_STRING:
                {
                    const char* string_value = nullptr;
                    annotations.get_annotation_param_value<const char*>( a, p, string_value);
                    std::cout << "\"" << string_value << "\"\n";
                    break;
                }

                case mi::neuraylib::IType::TK_FLOAT:
                {
                    mi::Float32 float_value = 0.0f;
                    annotations.get_annotation_param_value<mi::Float32>( a, p, float_value);
                    std::cout << float_value << "\n";
                    break;
                }

                default:
                {
                    // type not handled in this example
                    std::cout << "Address: " << mi::base::Handle<const mi::neuraylib::IValue>(
                        annotations.get_annotation_param_value( a, p)).get() << "\n";
                    break;
                }
            }
        }
    }

    // some other convenient helpers
    std::cout << "\n";
    std::cout << "Index of 'hard_range': " << annotations.get_annotation_index(
        "::anno::hard_range(float,float)") << "\n";
    std::cout << "Index of 'foo': " << static_cast<mi::Sint32>( annotations.get_annotation_index(
        "::anno::foo(int)")) << " (which is not present)\n";

    const char* descValue = nullptr;
    mi::Sint32 res = annotations.get_annotation_param_value_by_name<const char*>(
        "::anno::description(string)", 0, descValue);
    std::cout << "Value of 'description': \"" << ( res == 0 ? descValue : "nullptr") << "\"\n";

    mi::Sint32 fooValue = 0;
    res = annotations.get_annotation_param_value_by_name<mi::Sint32>(
        "::anno::foo(int)", 0, fooValue);
    if ( res != 0)
        std::cout << "Value of 'foo' not found (annotation is not present)\n";
    
    std::cout << std::endl;
}

// Creates a variant of the example material with different defaults.
void create_variant( mi::neuraylib::INeuray* neuray, mi::neuraylib::ITransaction* transaction)
{
    mi::base::Handle<mi::neuraylib::IMdl_factory> mdl_factory(
        neuray->get_api_component<mi::neuraylib::IMdl_factory>());
    mi::base::Handle<mi::neuraylib::IValue_factory> value_factory(
        mdl_factory->create_value_factory( transaction));
    mi::base::Handle<mi::neuraylib::IExpression_factory> expression_factory(
        mdl_factory->create_expression_factory( transaction));

    // Prepare new defaults as clone of the current arguments of the material instance.
    mi::base::Handle<const mi::neuraylib::IMaterial_instance> material_instance(
        transaction->access<mi::neuraylib::IMaterial_instance>( material_instance_name));
    mi::base::Handle<const mi::neuraylib::IExpression_list> arguments(
        material_instance->get_arguments());
    mi::base::Handle<mi::neuraylib::IExpression_list> defaults(
        expression_factory->clone( arguments.get()));

    // Create an ::anno::description annotation.
    mi::base::Handle<mi::neuraylib::IValue> anno_arg_value(
        value_factory->create_string(
            "a variant of ::nvidia::sdk_examples::tutorials::example_material"
            " with different defaults"));
    mi::base::Handle<mi::neuraylib::IExpression> anno_arg_expression(
        expression_factory->create_constant( anno_arg_value.get()));
    mi::base::Handle<mi::neuraylib::IExpression_list> anno_args(
        expression_factory->create_expression_list());
    anno_args->add_expression( "description", anno_arg_expression.get());
    mi::base::Handle<mi::neuraylib::IAnnotation> anno(
        expression_factory->create_annotation( "::anno::description(string)", anno_args.get()));

    // Create an ::anno::hard_range annotation.
    mi::base::Handle<mi::neuraylib::IValue> range_min_value( value_factory->create_float( 1.0));
    mi::base::Handle<mi::neuraylib::IValue> range_max_value( value_factory->create_float( 1024.0));
    mi::base::Handle<mi::neuraylib::IExpression> range_min_expression(
        expression_factory->create_constant( range_min_value.get()));
    mi::base::Handle<mi::neuraylib::IExpression> range_max_expression(
        expression_factory->create_constant( range_max_value.get()));
    mi::base::Handle<mi::neuraylib::IExpression_list> range_args(
        expression_factory->create_expression_list());
    range_args->add_expression( "min", range_min_expression.get());
    range_args->add_expression( "max", range_max_expression.get());
    mi::base::Handle<mi::neuraylib::IAnnotation> range_anno(
        expression_factory->create_annotation(
            "::anno::hard_range(float,float)", range_args.get()));

    // Add annotations to a annotation block
    mi::base::Handle<mi::neuraylib::IAnnotation_block> anno_block(
        expression_factory->create_annotation_block());
    anno_block->add_annotation( anno.get());
    anno_block->add_annotation( range_anno.get());

    // Set up variant data: an array with a single element of type Variant_data for variant
    // name, prototype name, new defaults, and the annotation created above.
    mi::base::Handle<mi::IArray> variant_data(
        transaction->create<mi::IArray>( "Variant_data[1]"));
    mi::base::Handle<mi::IStructure> variant(
        variant_data->get_value<mi::IStructure>( static_cast<mi::Size>( 0)));
    mi::base::Handle<mi::IString> variant_name(
        variant->get_value<mi::IString>( "variant_name"));
    variant_name->set_c_str( "green_example_material") ;
    mi::base::Handle<mi::IString> prototype_name(
        variant->get_value<mi::IString>( "prototype_name"));
    prototype_name->set_c_str( "mdl::nvidia::sdk_examples::tutorials::example_material");
    check_success( variant->set_value( "defaults", defaults.get()) == 0);
    check_success( variant->set_value( "annotations", anno_block.get()) == 0);

    // print the annotations just to illustrate the convince helper
    print_annotations( anno_block.get());

    // Create the variant.
    check_success( mdl_factory->create_variants(
        transaction, "::variants", variant_data.get()) == 0);

    // Instantiate the material definition of the variant.
    mi::base::Handle<const mi::neuraylib::IMaterial_definition> material_definition(
    transaction->access<mi::neuraylib::IMaterial_definition>(
        "mdl::variants::green_example_material"));
    mi::Sint32 result = 0;
    material_instance = material_definition->create_material_instance( 0, &result);
    check_success( result == 0);

    std::cout << "Dumping material instance with defaults of material definition "
        "\"mdl::variants::green_example_material\":" << std::endl;
    dump_instance( expression_factory.get(), material_instance.get(), std::cout);

    // Export the variant.
    mi::base::Handle<mi::neuraylib::IMdl_compiler> mdl_compiler(
        neuray->get_api_component<mi::neuraylib::IMdl_compiler>());
    check_success( mdl_compiler->export_module( transaction, "mdl::variants", "variants.mdl") == 0);
}

int main( int /*argc*/, char* /*argv*/[])
{
    // Access the MDL SDK
    mi::base::Handle<mi::neuraylib::INeuray> neuray( load_and_get_ineuray());
    check_success( neuray.is_valid_interface());

    // Configure the MDL SDK
    configure( neuray.get());

    // Start the MDL SDK
    mi::Sint32 result = neuray->start();
    check_start_success( result);

    {
        mi::base::Handle<mi::neuraylib::IDatabase> database(
            neuray->get_api_component<mi::neuraylib::IDatabase>());
        mi::base::Handle<mi::neuraylib::IScope> scope( database->get_global_scope());
        mi::base::Handle<mi::neuraylib::ITransaction> transaction( scope->create_transaction());

        {
            // Instantiate a material and function definition
            instantiate_definitions( neuray.get(), transaction.get());

            // Change the arguments of the instantiated definitions
            change_arguments( neuray.get(), transaction.get());

            // Create a variant of the diffuse material with different defaults.
            create_variant( neuray.get(), transaction.get());
        }

        transaction->commit();
    }

    // Shut down the MDL SDK
    check_success( neuray->shutdown() == 0);
    neuray = 0;

    // Unload the MDL SDK
    check_success( unload());

    keep_console_open();
    return EXIT_SUCCESS;
}

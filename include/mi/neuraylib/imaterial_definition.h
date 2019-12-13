/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Scene element Material_definition

#ifndef MI_NEURAYLIB_IMATERIAL_DEFINITION_H
#define MI_NEURAYLIB_IMATERIAL_DEFINITION_H

#include <mi/neuraylib/iexpression.h>
#include <mi/neuraylib/iscene_element.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_mdl_elements
@{
*/

class IMaterial_instance;

/// This interface represents a material definition.
///
/// A material definition describes the formal structure of a material instance, i.e. the number,
/// types, names, and defaults of its parameters. The #create_material_instance() method allows to
/// create material instances based on this material definition.
///
/// \see #mi::neuraylib::IMaterial_instance, #mi::neuraylib::IModule,
///      #mi::neuraylib::Definition_wrapper
class IMaterial_definition : public
    mi::base::Interface_declare<0x73753e3d,0x62e4,0x41a7,0xa8,0xf5,0x37,0xeb,0xda,0xd9,0x01,0xd9,
                                neuraylib::IScene_element>
{
public:
    /// Returns the DB name of the module containing this material definition.
    ///
    /// The type of the module is #mi::neuraylib::IModule.
    virtual const char* get_module() const = 0;

    /// Returns the MDL name of the material definition.
    ///
    /// \note The MDL name of the material definition is different from the name of the DB element.
    ///       Use #mi::neuraylib::ITransaction::name_of() to obtain the name of the DB element.
    ///
    /// \return         The MDL name of the material definition.
    virtual const char* get_mdl_name() const = 0;

    /// Returns the DB name of the prototype, or \c NULL if this material definition is not a
    /// variant.
    virtual const char* get_prototype() const = 0;

    /// Indicates whether the material definition is exported by its module.
    virtual bool is_exported() const = 0;

    /// Returns the number of parameters.
    virtual Size get_parameter_count() const = 0;

    /// Returns the name of the parameter at \p index.
    ///
    /// \param index    The index of the parameter.
    /// \return         The name of the parameter, or \c NULL if \p index is out of range.
    virtual const char* get_parameter_name( Size index) const = 0;

    /// Returns the index position of a parameter.
    ///
    /// \param name     The name of the parameter.
    /// \return         The index of the parameter, or -1 if \p name is invalid.
    virtual Size get_parameter_index( const char* name) const = 0;

    /// Returns the types of all parameters.
    virtual const IType_list* get_parameter_types() const = 0;

    /// Returns the defaults of all parameters.
    ///
    /// \note Not all parameters have defaults. Hence, the indices in the returned expression list
    ///       do not necessarily coincide with the parameter indices of this definition. Therefore,
    ///       defaults should be retrieved via the name of the parameter instead of its index.
    virtual const IExpression_list* get_defaults() const = 0;

    /// Returns the enable_if conditions of all parameters.
    ///
    /// \note Not all parameters have a condition. Hence, the indices in the returned expression
    ///       list do not necessarily coincide with the parameter indices of this definition.
    ///       Therefore, conditions should be retrieved via the name of the parameter instead of
    ///       its index.
    virtual const IExpression_list* get_enable_if_conditions() const = 0;

    /// Returns the number of other parameters whose enable_if condition might depend on the
    /// argument of the given parameter.
    ///
    /// \param index    The index of the parameter.
    /// \return         The number of other parameters whose enable_if condition depends on this
    ///                 parameter argument.
    virtual Size get_enable_if_users( Size index) const = 0;

    /// Returns the index of a parameter whose enable_if condition might depend on the
    /// argument of the given parameter.
    ///
    /// \param index    The index of the parameter.
    /// \param u_index  The index of the enable_if user.
    /// \return         The index of a parameter whose enable_if condition depends on this
    ///                 parameter argument, or ~0 if indexes are out of range.
    virtual Size get_enable_if_user( Size index, Size u_index) const = 0;

    /// Returns the annotations of the material definition itself, or \c NULL if there are no such
    /// annotations.
    virtual const IAnnotation_block* get_annotations() const = 0;

    /// Returns the annotations of all parameters.
    ///
    /// \note Not all parameters have annotations. Hence, the indices in the returned annotation
    ///       list do not necessarily coincide with the parameter indices of this definition.
    ///       Therefore, annotation blocks should be retrieved via the name of the parameter
    ///       instead of its index.
    virtual const IAnnotation_list* get_parameter_annotations() const = 0;

    /// Creates a new material instance.
    ///
    /// \param arguments    The arguments of the created material instance. \n
    ///                     Arguments for parameters without default are mandatory, otherwise
    ///                     optional. The type of an argument must match the corresponding parameter
    ///                     type. Any argument missing in \p arguments will be set to the default of
    ///                     the corresponding parameter. \n
    ///                     Note that the expressions in \p arguments are copied. This copy
    ///                     operation is a deep copy, e.g., DB elements referenced in call
    ///                     expressions are also copied. \n
    ///                     \c NULL is a valid argument which is handled like an empty expression
    ///                     list.
    /// \param[out] errors  An optional pointer to an #mi::Sint32 to which an error code will be
    ///                     written. The error codes have the following meaning:
    ///                     -  0: Success.
    ///                     - -1: An argument for a non-existing parameter was provided in
    ///                           \p arguments.
    ///                     - -2: The type of an argument in \p arguments does not have the correct
    ///                           type, see #get_parameter_types().
    ///                     - -3: A parameter that has no default was not provided with an argument
    ///                           value.
    ///                     - -4: The definition can not be instantiated because it is not exported.
    ///                     - -5: A parameter type is uniform, but the corresponding argument has a
    ///                           varying return type.
    ///                     - -6: An argument expression is not a constant nor a call.
    ///                     - -8: One of the parameter types is uniform, but the corresponding
    ///                           argument or default is a call expression and the return type of
    ///                           the called function definition is effectively varying since the
    ///                           function definition itself is varying.
    /// \return             The created material instance, or \c NULL in case of errors.
    virtual IMaterial_instance* create_material_instance(
        const IExpression_list* arguments, Sint32* errors = 0) const = 0;

    /// Returns the resolved file name of the thumbnail image for this material definition.
    ///
    /// The function first checks for a thumbnail annotation. If the annotation is provided, 
    /// it uses the 'name' argument of the annotation and resolves that in the MDL search path.
    /// If the annotation is not provided or file resolution fails, it checks for a file
    /// module_name.material_name.png next to the MDL module.
    /// In case this cannot be found either \c NULL is returned.
    ///
    virtual const char* get_thumbnail() const = 0;
};

/*@}*/ // end group mi_neuray_mdl_elements

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IMATERIAL_DEFINITION_H

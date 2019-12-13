/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Scene element Material_instance

#ifndef MI_NEURAYLIB_IMATERIAL_INSTANCE_H
#define MI_NEURAYLIB_IMATERIAL_INSTANCE_H

#include <mi/neuraylib/iexpression.h>
#include <mi/neuraylib/iscene_element.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_mdl_elements
@{
*/

class ICompiled_material;
class IMdl_execution_context;

/// This interface represents a material instance.
///
/// A material instance is a concrete instance of a formal material definition, with a fixed set of
/// arguments (possibly the defaults of the material definition). Material instances can be created
/// from material definitions using
/// #mi::neuraylib::IMaterial_definition::create_material_instance().
///
/// \see #mi::neuraylib::IMaterial_definition, #mi::neuraylib::Argument_editor
class IMaterial_instance : public
    mi::base::Interface_declare<0x037ec156,0x281d,0x466a,0xa1,0x56,0x3e,0xd6,0x83,0xe9,0x5a,0x00,
                                neuraylib::IScene_element>
{
public:
    /// Returns the DB name of the corresponding material definition.
    ///
    /// The type of the material definition is #mi::neuraylib::IMaterial_definition.
    ///
    /// \note The DB name of the material definition is different from its MDL name (see
    ///       #get_mdl_material_definition()).
    virtual const char* get_material_definition() const = 0;

    /// Returns the MDL name of the corresponding material definition.
    ///
    /// \note The MDL name of the material definition is different from the name of the DB element
    ///       (see #get_material_definition()).
    virtual const char* get_mdl_material_definition() const = 0;

    /// Returns the number of parameters.
    virtual Size get_parameter_count() const = 0;

    /// Returns the name of the parameter at \p index.
    ///
    /// \param index        The index of the parameter.
    /// \return             The name of the parameter, or \c NULL if \p index is out of range.
    virtual const char* get_parameter_name( Size index) const = 0;

    /// Returns the index position of a parameter.
    ///
    /// \param name         The name of the parameter.
    /// \return             The index of the parameter, or -1 if \p name is invalid.
    virtual Size get_parameter_index( const char* name) const = 0;

    /// Returns the types of all parameters.
    virtual const IType_list* get_parameter_types() const = 0;

    /// Returns all arguments.
    virtual const IExpression_list* get_arguments() const = 0;

    /// Sets multiple arguments.
    ///
    /// \param arguments    The arguments. Note that the expressions are copied. This copy operation
    ///                     is a shallow copy, e.g., DB elements referenced in call expressions are
    ///                     \em not copied.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: One of the parameters in \p arguments does not exist.
    ///                     - -3: One of the argument types does not match the corresponding
    ///                           parameter type.
    ///                     - -4: The material instance is immutable (because it appears in a
    ///                           default of a material definition).
    ///                     - -5: One of the parameter types is uniform, but the corresponding
    ///                           argument type is varying.
    ///                     - -6: One of the arguments is not a constant nor a call.
    ///                     - -7: One of the arguments contains references to DB elements in a scope
    ///                           that is more private scope than the scope of this material
    ///                           instance.
    ///                     - -8: One of the parameter types is uniform, but the corresponding
    ///                           argument is a call expression and the return type of the
    ///                           called function definition is effectively varying since the
    ///                           function definition itself is varying.
    virtual Sint32 set_arguments( const IExpression_list* arguments) = 0;

    /// Sets the argument at \p index.
    ///
    /// \param index        The index of the argument.
    /// \param argument     The argument. Note that the expression is copied. This copy operation
    ///                     is a shallow copy, e.g., DB elements referenced in call expressions are
    ///                     \em not copied.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: Parameter \p index does not exist.
    ///                     - -3: The argument type does not match the parameter type.
    ///                     - -4: The material instance is immutable (because it appears in a
    ///                           default of a material definition).
    ///                     - -5: The parameter type is uniform, but the argument type is varying.
    ///                     - -6: The argument expression is not a constant nor a call.
    ///                     - -7: The argument contains references to DB elements in a scope that is
    ///                           more private scope than the scope of this material instance.
    ///                     - -8: The parameter types is uniform, but the argument is a call
    ///                           expression and the return type of the called function definition
    ///                           is effectively varying since the function definition itself is
    ///                           varying.
    virtual Sint32 set_argument( Size index, const IExpression* argument) = 0;

    /// Sets an argument identified by name.
    ///
    /// \param name         The name of the parameter.
    /// \param argument     The argument. Note that the expression is copied. This copy operation
    ///                     is a shallow copy, e.g., DB elements referenced in call expressions are
    ///                     \em not copied.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: Parameter \p name does not exist.
    ///                     - -3: The argument type does not match the parameter type.
    ///                     - -4: The material instance is immutable (because it appears in a
    ///                           default of a material definition).
    ///                     - -5: The parameter type is uniform, but the argument type is varying.
    ///                     - -6: The argument expression is not a constant nor a call.
    ///                     - -7: The argument contains references to DB elements in a scope that is
    ///                           more private scope than the scope of this material instance.
    ///                     - -8: The parameter types is uniform, but the argument is a call
    ///                           expression and the return type of the called function definition
    ///                           is effectively varying since the function definition itself is
    ///                           varying.
    virtual Sint32 set_argument( const char* name, const IExpression* argument) = 0;

    /// Various options for the creation of compiled materials.
    ///
    /// \see #create_compiled_material()
    enum Compilation_options {
        DEFAULT_OPTIONS   = 0, ///< Default compilation options (e.g., instance compilation).
        CLASS_COMPILATION = 1, ///< Selects class compilation instead of instance compilation.
        COMPILATION_OPTIONS_FORCE_32_BIT = 0xffffffffU // Undocumented, for alignment only
    };

    mi_static_assert( sizeof( Compilation_options) == sizeof( mi::Uint32));

    /// Creates a compiled material.
    ///
    /// \param flags                       A bitmask of flags of type #Compilation_options.
    /// \param mdl_meters_per_scene_unit   The conversion ratio between meters and scene units for
    ///                                    this material.
    /// \param mdl_wavelength_min          The smallest supported wavelength. Typical value: 380.
    /// \param mdl_wavelength_max          The largest supported wavelength. Typical value: 780.
    /// \param[out] errors                 An optional pointer to an #mi::Sint32 to which an error
    ///                                    code will be written. The error codes have the following
    ///                                    meaning:
    ///                                    -  0: Success.
    ///                                    - -1: Type mismatch, call of an unsuitable DB element, or
    ///                                          call cycle in the graph of this material instance.
    ///                                    - -2: The thin-walled material instance has different
    ///                                          transmission for surface and backface.
    ///                                    - -3: An argument type of the graph of this material
    ///                                          instance is varying but the corresponding parameter
    ///                                          type is uniform.
    /// \return                            The corresponding compiled material, or \c NULL in case
    ///                                    of failure.
    virtual ICompiled_material* deprecated_create_compiled_material(
        Uint32 flags,
        Float32 mdl_meters_per_scene_unit,
        Float32 mdl_wavelength_min,
        Float32 mdl_wavelength_max,
        Sint32* errors = 0) const = 0;

#ifdef MI_NEURAYLIB_DEPRECATED_9_1
    /// Creates a compiled material.
    ///
    /// \param flags                       A bitmask of flags of type #Compilation_options.
    /// \param mdl_meters_per_scene_unit   The conversion ratio between meters and scene units for
    ///                                    this material.
    /// \param mdl_wavelength_min          The smallest supported wavelength. Typical value: 380.
    /// \param mdl_wavelength_max          The largest supported wavelength. Typical value: 780.
    /// \param[out] errors                 An optional pointer to an #mi::Sint32 to which an error
    ///                                    code will be written. The error codes have the following
    ///                                    meaning:
    ///                                    -  0: Success.
    ///                                    - -1: Type mismatch, call of an unsuitable DB element, or
    ///                                          call cycle in the graph of this material instance.
    ///                                    - -2: The thin-walled material instance has different
    ///                                          transmission for surface and backface.
    ///                                    - -3: An argument type of the graph of this material
    ///                                          instance is varying but the corresponding parameter
    ///                                          type is uniform.
    /// \return                            The corresponding compiled material, or \c NULL in case
    ///                                    of failure.
    ICompiled_material* create_compiled_material(
        Uint32 flags,
        Float32 mdl_meters_per_scene_unit,
        Float32 mdl_wavelength_min,
        Float32 mdl_wavelength_max,
        Sint32* errors = 0) const
    {
        return deprecated_create_compiled_material(
            flags,
            mdl_meters_per_scene_unit,
            mdl_wavelength_min,
            mdl_wavelength_max,
            errors);
    }
#endif

    /// Creates a compiled material.
    ///
    /// \param flags                       A bitmask of flags of type #Compilation_options.
    /// \param[inout] context              An optional pointer to an
    ///                                    #mi::neuraylib::IMdl_execution_context which can be used
    ///                                    to pass compilation options to the MDL compiler. The
    ///                                    following options are supported for this operation:
    ///                                    - Float32 "meters_per_scene_unit": The conversion
    ///                                      ratio between meters and scene units for this
    ///                                      material. Default: 1.0f.
    ///                                    - Float32 "wavelength_min": The smallest
    ///                                      supported wavelength. Default: 380.0f.
    ///                                    - Float32 "wavelength_max": The largest supported
    ///                                      wavelength. Default: 780.0f.
    ///                                    - bool "fold_ternary_on_df": Fold all ternary operators
    ///                                      of *df types, even in class compilation mode.
    ///                                      Default: false.
    ///                                    During material compilation, messages like errors and
    ///                                    warnings will be passed to the context for
    ///                                    later evaluation by the caller.
    /// \return                            The corresponding compiled material, or \c NULL in case
    ///                                    of failure.
    virtual ICompiled_material* create_compiled_material(
        Uint32 flags,
        IMdl_execution_context* context = 0) const = 0;

    /// Indicates, if this material instance acts as a default argument of a material or
    /// function definition.
    ///
    /// Defaults are immutable, their arguments cannot be changed and they cannot be used
    /// in call expressions.
    ///
    /// \return true, if this material instance is a default, false otherwise.
    virtual bool is_default() const = 0;
};

/*@}*/ // end group mi_neuray_mdl_elements

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IMATERIAL_INSTANCE_H

/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for MDL related settings.

#ifndef MI_NEURAYLIB_IMDL_CONFIGURATION_H
#define MI_NEURAYLIB_IMDL_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface can be used to query and change the MDL configuration.
class IMdl_configuration : public
    mi::base::Interface_declare<0x2657ec0b,0x8a40,0x46c5,0xa8,0x3f,0x2b,0xb5,0x72,0xa0,0x8b,0x9c>
{
public:

    /// Defines, if the SDK should automatically insert a cast operator when an
    /// #mi::neuraylib::IExpression of a different but compatible type is passed to an argument of
    /// MDL instances of type #mi::neuraylib::IFunction_call or #mi::neuraylib::IMaterial_instance.
    /// If set to \c false, such an assignment fails, unless the cast has been inserted explicitly.
    /// See also #mi::neuraylib::IExpression_factory::create_cast().
    /// Default: \c true.
    ///
    /// \param value    \c True to enable the feature, \c false otherwise.
    /// \return
    ///                -  0: Success.
    ///                - -1: The method cannot be called at this point of time.
    virtual Sint32 set_implicit_cast_enabled(bool value) = 0;

    /// Returns, if the SDK is supposed to insert the cast operator for compatible types automatically.
    ///
    /// \see #set_implicit_cast_enabled()
    virtual bool get_implicit_cast_enabled() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IMDL_CONFIGURATION_H

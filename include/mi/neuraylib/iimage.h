/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Scene element Image

#ifndef MI_NEURAYLIB_IIMAGE_H
#define MI_NEURAYLIB_IIMAGE_H

#include <mi/neuraylib/iscene_element.h>

namespace mi {

class IArray;

namespace neuraylib {

class ICanvas;
class IReader;

/** \defgroup mi_neuray_misc Miscellaneous
    \ingroup mi_neuray_scene_element

    Miscellaneous scene graph elements, for example, textures, light profiles, BSDF measurements,
    or decals.
*/

/** \addtogroup mi_neuray_misc
@{
*/

/// This interface represents a pixel image file. It supports different pixel types, 2D and 3D image
/// data, and mipmap levels. Its main usage is in textures, see the #mi::neuraylib::ITexture class.
///
/// The image coordinate system has its origin in the lower left corner in the case of 2D image
/// data.
///
/// \par Editing and copying an image
///
/// Note that editing an existing image has unusual semantics that differ from all other DB
/// elements. Usually, when editing a database element, an identical copy of the database element is
/// created (the existing one cannot be used because it might be needed for other transactions,
/// other scopes, or in case the transaction is aborted). For images, this implies a copy of all the
/// pixel data which is very expensive.
///
/// There are only two mutable methods on this interface, #reset_file() and #set_from_canvas();
/// all other methods are const. Both methods eventually replace the entire pixel data anyway.
/// Therefore, when an image is edited, the pixel data is not copied, but replaced by a dummy image
/// of size 1x1. This approach saves the unneeded, but expensive copy of the original pixel data.
/// When afterwards one of two methods above is called, the image uses the correct pixel data
/// again.
///
/// Note that this also affects the results from methods like #resolution_x(), etc. (if you want
/// to know the resolution of an existing image without changing it, you should access the image,
/// not edit it). Furthermore, you might end up with the dummy image if you do not call
/// #reset_file() or #set_from_canvas() (or if these methods fail).
///
/// Note that using the transaction's copy function has the same semantics when used on an image.
/// Thus after copying it is necessary to use either #reset_file() or #set_from_canvas() on the
/// copy.
///
class IImage :
    public base::Interface_declare<0xca59b977,0x30ee,0x4172,0x91,0x53,0xb7,0x70,0x2c,0x6b,0x3a,0x76,
                                   neuraylib::IScene_element>
{
public:
    /// Sets the image to a file identified by \p filename.
    ///
    /// Note that support for a given image format requires an image plugin capable of handling
    /// that format.
    ///
    /// \return
    ///                       -  0: Success.
    ///                       - -1: Invalid parameters (\c NULL pointer).
    ///                       - -2: Failure to resolve the given filename, e.g., the file does not
    ///                             exist.
    ///                       - -3: Failure to open the file.
    ///                       - -4: No image plugin found to handle the file.
    ///                       - -5: The image plugin failed to import the file.
    virtual Sint32 reset_file( const char* filename) = 0;

    /// Sets the image to the data provided by a reader.
    ///
    /// \param reader         The reader that provides the data for the image. The reader needs to
    ///                       support absolute access.
    /// \param image_format   The image format of the data, e.g., \c "jpg". Note that support for a
    ///                       given image format requires an image plugin capable of handling that
    ///                       format.
    /// \return
    ///                       -  0: Success.
    ///                       - -1: Invalid parameters (\c NULL pointer).
    ///                       - -3: The reader does not support absolute access.
    ///                       - -4: No image plugin found to handle the data.
    ///                       - -5: The image plugin failed to import the data.
    virtual Sint32 reset_reader( IReader* reader, const char* image_format) = 0;

    /// Sets the image to the uv-tile data provided by an array of readers.
    ///
    /// \param reader         The array of readers that provide the data for the image. 
    ///                       The reader needs to support absolute access.
    /// \param image_format   The image format of the data, e.g., \c "jpg". Note that support for a
    ///                       given image format requires an image plugin capable of handling that
    ///                       format.
    /// \return
    ///                       -  0: Success.
    ///                       - -1: Invalid parameters (\c NULL pointer).
    ///                       - -3: The reader does not support absolute access.
    ///                       - -4: No image plugin found to handle the data.
    ///                       - -5: The image plugin failed to import the data.
    virtual Sint32 reset_reader( IArray* reader, const char* image_format) = 0;

    /// Returns the resolved file name of the file containing the image.
    ///
    /// The method returns \c NULL if there is no file associated with the image, e.g., after
    /// default construction, calls to #set_from_canvas(), or failures to resolve the file name
    /// passed to #reset_file().
    ///
    /// \see #get_original_filename()
    virtual const char* get_filename( Uint32 uvtile_id = 0) const = 0;

    /// Returns the unresolved file as passed to #reset_file().
    ///
    /// The method returns \c NULL after default construction or calls to #set_from_canvas().
    ///
    /// \see #get_filename()
    virtual const char* get_original_filename() const = 0;

    /// Sets the pixels of this image based on the passed canvas (without sharing).
    ///
    /// \param canvas   The pixel data to be used by this image. Note that the pixel data is copied,
    ///                 not shared. If sharing is intended use
    ///                 #mi::neuraylib::IImage::set_from_canvas(mi::neuraylib::ICanvas*,bool)
    ///                 instead.
    /// \return         \c true if the pixel data of this image has been set correctly, and
    ///                 \c false otherwise.
    virtual bool set_from_canvas( const ICanvas* canvas) = 0;

    /// Sets the pixels of this image based on the passed canvas (possibly sharing the pixel data).
    ///
    /// \param canvas   The pixel data to be used by this image.
    /// \param shared   If \c false (the default), the pixel data is copied from \c canvas and the
    ///                 method does the same as
    ///                 #mi::neuraylib::IImage::set_from_canvas(const mi::neuraylib::ICanvas*).
    ///                 If set to \c true, the image uses the canvas directly (doing reference
    ///                 counting on the canvas pointer). You must not modify the canvas content
    ///                 after this call.
    /// \return         \c true if the pixel data of this image has been set correctly, and
    ///                 \c false otherwise.
    virtual bool set_from_canvas( ICanvas* canvas, bool shared = false) = 0;

    /// Sets the pixels of the uv-tiles of this image based on the passed canvas (without sharing).
    ///
    /// \param uvtiles  The uv-tile pixel data to be used by this image. Note that the pixel data 
    ///                 is copied, not shared. If sharing is intended use
    ///                 #mi::neuraylib::IImage::set_from_canvas(mi::IArray*,bool)
    ///                 instead.
    /// \return         \c true if the pixel data of this image has been set correctly, and
    ///                 \c false otherwise.
    virtual bool set_from_canvas( const IArray* uvtiles) = 0;

    /// Sets the pixels of the uv-tiles of this image based on the passed canvas 
    /// (possibly sharing the pixel data).
    ///
    /// \param uvtiles  The uv-tile pixel data to be used by this image.
    /// \param shared   If \c false (the default), the pixel data is copied from \c canvas and the
    ///                 method does the same as
    ///                 #mi::neuraylib::IImage::set_from_canvas(const mi::neuraylib::ICanvas*).
    ///                 If set to \c true, the image uses the canvas directly (doing reference
    ///                 counting on the canvas pointer). You must not modify the canvas content
    ///                 after this call.
    /// \return         \c true if the pixel data of this image has been set correctly, and
    ///                 \c false otherwise.
    virtual bool set_from_canvas( IArray* uvtiles, bool shared = false) = 0;

    /// Returns a canvas with the pixel data of the image.
    ///
    /// Note that it is not possible to manipulate the pixel data.
    ///
    /// \param level       The desired mipmap level. Level 0 is the highest resolution.
    /// \param uvtile_id   The uv-tile id of the canvas.
    /// \return            A canvas pointing to the pixel data of the image, or \c NULL in case of
    ///                    failure, e.g. because of an invalid tile id.
    virtual const ICanvas* get_canvas( Uint32 level = 0, Uint32 uvtile_id = 0) const = 0;

    /// Returns the pixel type of the image.
    ///
    /// \param uvtile_id   The uv-tile id of the canvas to get the pixel type for.
    /// \return            The pixel type or 0 in case of an invalid tile id.
    /// See \ref mi_neuray_types for a list of supported pixel types.
    virtual const char* get_type( Uint32 uvtile_id = 0) const = 0 ;

    /// Returns the number of levels in the mipmap pyramid.
    ///
    /// \param uvtile_id   The uv-tile id of the canvas to get the number of levels for.
    /// \return            The number of levels or -1 in case of an invalid tile id.
    virtual Uint32 get_levels( Uint32 uvtile_id = 0) const = 0;

    /// Returns the horizontal resolution of the image.
    ///
    /// \param level       The desired mipmap level. Level 0 is the highest resolution.
    /// \param uvtile_id   The uv-tile id of the canvas to get the resolution for.
    /// \return            The horizontal resolution or -1 in case of an invalid tile id.
    virtual Uint32 resolution_x( Uint32 level = 0, Uint32 uvtile_id = 0) const = 0;

    /// Returns the vertical resolution of the image.
    ///
    /// \param level       The desired mipmap level. Level 0 is the highest resolution.
    /// \param uvtile_id   The uv-tile id of the canvas to get the resolution for.
    /// \return            The vertical resolution or -1 in case of an invalid tile id.
    virtual Uint32 resolution_y( Uint32 level = 0, Uint32 uvtile_id = 0) const = 0;

    /// Returns the number of layers of the 3D image.
    ///
    /// \param level       The desired mipmap level. Level 0 is the highest resolution.
    /// \param uvtile_id   The uv-tile id of the canvas to get the resolution for.
    /// \return            The number of layers or -1 in case of an invalid tile id.
    virtual Uint32 resolution_z( Uint32 level = 0, Uint32 uvtile_id = 0) const = 0;

    /// Returns the number of uv-tiles of the image.
    ///
    virtual Size get_uvtile_length() const = 0;

    /// Returns the u and v tile indices of the uv-tile at the given index.
    ///
    /// \param uvtile_id   The uv-tile id of the canvas.
    /// \param u           The u-component of the uv-tile
    /// \param v           The v-component of the uv-tile
    /// \return            0 on success, -1 if uvtile_id is out of range.
    virtual Sint32 get_uvtile_uv( Uint32 uvtile_id, Sint32& u, Sint32& v) const = 0;

    // Returns the uvtile-id corresponding to the tile at u,v.
    ///
    /// \param u           The u-component of the uv-tile
    /// \param v           The v-component of the uv-tile
    /// \return The uvtile-id or -1 of there is no tile with the given coordinates.
    virtual Uint32 get_uvtile_id( Sint32 u, Sint32 v) const = 0;

    // Returns true if this image represents a uvtile/udim image sequence.
    virtual bool is_uvtile() const = 0;
};

/*@}*/ // end group mi_neuray_misc

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IIMAGE_H

<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Doxygen groups

@defgroup API
@brief The high-level API and recommended way of using `GridFormat`
@details The high-level API exposes all available file formats and the generic
         GridFormat::Writer, which takes an instance of one of the formats and a
         grid that is to be written. It is recommended to use the provided file
         format selectors for construction of a GridFormat::Writer. As an example,
         to construct a writer for the .vtu file format for your `grid`, you may write
         @code{.cpp}
             GridFormat::Writer writer{GridFormat::vtu, grid};
         @endcode

         You can also let `GridFormat` select a default for you:
         @code{.cpp}
             GridFormat::Writer writer{GridFormat::default_for(grid), grid};
         @endcode

         Some file formats take configuration options. For instance, the .vtu file
         format allows you to specify (among other options) an encoder to be used:
         @code{.cpp}
             GridFormat::Writer writer{
                GridFormat::vtu({.encoder = GridFormat::Encoding::base64}),
                grid
             };
         @endcode

         For more information on these options, see GridFormat::FileFormat::VTU.
         Note that all writers take the given grid by reference, and thus, their
         validity is bound to the lifetime of the grid.

@defgroup Encoding
@brief Encoders that can be used for I/O.

@defgroup Compression
@brief Compressors that can be used to compress data before writing.

@defgroup VTK
@brief Classes & functions related to writing VTK files.

@defgroup Grid
@brief Classes & functions related to grids and operations on them.

<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Doxygen groups

@defgroup API
@brief The high-level API and recommended way of using `GridFormat`
@details The high-level API exposes all available @ref FileFormats and the generic
         GridFormat::Writer (and GridFormat::Reader, see below), which takes an
         instance of one of the formats and a grid that is to be written. It is
         recommended to use the provided @ref FormatSelectors for construction of
         a GridFormat::Writer. As an example, to construct a writer for the .vtu file
         format for your `grid`, you may write
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

         The GridFormat::Reader can be constructed without any format argument, in
         which case the format will be deduced from the extension of the given file.
         For example, with the following code you can read both a .vtu and a .vti
         file with the same reader:
         @code{.cpp}
             GridFormat::Reader reader;
             reader.open("my_vti_file.vti");
             std::cout << "Number of cells in vti file: " << reader.number_of_cells() << std::endl;
             reader.open("my_vtu_file.vtu");
             std::cout << "Number of cells in vtu file: " << reader.number_of_cells() << std::endl;
         @endcode

         To create a reader for a specific file format, pass a format specifier to the constructor:
         @code{.cpp}
             GridFormat::Reader vti_reader{GridFormat::vti};
             reader.open("my_vti_file.vti");
             reader.open("my_vti_file_without_extension");
             // reader.open("my_vtu_file");  // this would not work
         @endcode

    @defgroup FileFormats File Formats
    @brief Classes representing the available file formats. For predefined selectors, see @ref FormatSelectors.
    @ingroup API
    @note Recent versions of VTK/ParaView are required to properly read and render the VTK-HDF file formats. The transient versions
        should be available in VTK>=9.3.0 and ParaView>=5.12.0 (see <a href="https://www.kitware.com/how-to-write-time-dependent-data-in-vtkhdf-files/">KitWare blog</a>). Moreover, several bug fixes related to reading cell
        and field data from VTK-HDF files for image grids have recently been incorporated into the main branch of VTK and may not
        be included in earlier versions.

    @defgroup FormatSelectors Format Selectors
    @brief Selector classes for all available file formats. For more details on the formats, see the @ref FileFormats classes.
           For examples on how to use them, see @ref API.
    @ingroup API

@defgroup Encoding
@brief Encoders that can be used for I/O.

@defgroup Compression
@brief Compressors that can be used to compress data before writing.

@defgroup VTK
@brief Classes & functions related to writing VTK files.

@defgroup Grid
@brief Classes & functions related to grids and operations on them.

@defgroup PredefinedTraits
@brief Traits specializations and helper classes for frameworks

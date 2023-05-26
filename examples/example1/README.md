<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 1: writing image data

This example illustrates how to write image data into the [.vti file format](https://examples.vtk.org/site/VTKFileFormats/#imagedata).
The example considers a data structure used to represent an image (see [image.hpp](./image.hpp)), for which we specialize the
[traits required for image grids](../../docs/traits.md#traits-for-image-grids) in the file [traits.hpp](./traits.hpp).

What you can see in this example is:

- how to specialize the grid traits for your own data structure
- how to statically check if your traits implementation is correct
- how to use the `GridFormat` API for writing grid files (i.e. using `GridFormat::Writer`)
- how to select the file format into which to write the grid
- how to choose options exposed by a particular file format (in this case the `.vti` file format)
    - select the encoder to be used
    - deactivate compression

<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 1: writing voxel data to image grid formats

<img alt="example1" src="https://github.com/dglaeser/gridformat/blob/main/examples/example1/img/result.png" width="50%"/>

This example illustrates how to register a custom data structure as
[`ImageGrid`](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#image-grid) and export the data into the
[.vti file format](https://examples.vtk.org/site/VTKFileFormats/#imagedata), which can be read, visualized and post-processed
with [ParaView](https://www.paraview.org/) (see the image above).
The example considers a simple data structure to represent data on 3d voxels, for which we specialize the
[traits required for image grids](../../docs/traits.md#traits-for-image-grids).

What you can see in this example is:

- how to specialize the grid traits for your own data structure
- how to statically check if your traits implementation is correct
- how to use the `GridFormat` API for writing grid files (i.e. using `GridFormat::Writer`)
- how to select the file format into which to write the grid
- how to add point fields, cell fields and metadata to a grid file
- how to choose options exposed by a particular file format (in this case the `.vti` file format)
    - select the encoder to be used
    - deactivate compression

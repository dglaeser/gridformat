<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Writing voxel data to image grid formats

<img alt="voxels-example" src="https://github.com/dglaeser/gridformat/blob/main/examples/voxels/img/result.png" width="50%"/>

This example illustrates how to register a custom data structure as
[`ImageGrid`](../../docs/pages/grid_concepts.md#image-grid) and export the data into the
[.vti file format](https://examples.vtk.org/site/VTKFileFormats/#imagedata), which can be read, visualized and post-processed
with [ParaView](https://www.paraview.org/) (see the image above).
The example considers a simple data structure to represent data on 3d voxels, for which we specialize the
[traits required for image grids](../../docs/pages/traits.md#traits-for-image-grids). This example also illustrates that when
using image grid file formats and in case you don't write out point data, you don't have to implement any traits related to grid points.

What you can see in this example:

- how to facilities of `std::ranges` to simplify traits specializations
- how to define empty (throwing) traits specializations for non-used features.
- how to choose options exposed by a particular file format (in this case the `.vti` file format)
    - select the encoder to be used
    - deactivate compression (active by default)

<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Writing a digital elevation model into `.vts` file format

<img alt="dem-example" src="https://github.com/dglaeser/gridformat/blob/main/examples/dem/img/result.png" width="80%"/>

This example shows the specialization of the traits required for the
[structured grid](../../docs/pages/grid_concepts.md#structured-grid) concept and illustrates how to write
[`.vts`](https://examples.vtk.org/site/VTKFileFormats/#structuredgrid) files. As an example, we consider a digital elevation
model (DEM), given as a 2D raster of discrete elevation data in terms of geographic coordinates. The considered data structure
for DEMs allows us to get the cartesian coordinates of any point in the raster, taking into account the elevation data. We use
this to write out the DEM such that it can be visualized with respect to the spheroid on which the geographic coordinates are
defined. The data structure for DEMs is defined in the header [`dem.hpp`](./dem.hpp), and the traits specializations can be
found in [`traits.hpp`](./traits.hpp).

This example shows:

- how to implement the traits required for the `StructuredGrid` concept
- how to write fields with a custom precision (e.g. to save space)
- how to select a compression algorithm to use for compressed VTK-XML output, and specify options for it

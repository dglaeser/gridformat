<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 2: writing a digital elevation model into .vts file format

<img alt="example2" src="https://github.com/dglaeser/gridformat/blob/feature/update-examples/examples/example2/img/result.png" width="80%"/>

In this example, we illustrate how the [`.vts`](https://examples.vtk.org/site/VTKFileFormats/#structuredgrid) file format for
[structured grids](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#structured-grid) can be used to write
digital elevation models, given as rasters of discrete elevation data over geographic coordinates, as a surface in 3d that can
be visualized in relation to the spheroid on which they are defined.

A data structure for digital elevation models is defined in the header [`dem.hpp`](./dem.hpp), and the traits specializations can be
found in [`traits.hpp`](./traits.hpp). What you can learn from this example is:

- how to implement the traits required for the `StructuredGrid` concept
- how to tell `GridFormat` to write fields with a custom precision (e.g. to save space)

<!-- SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Writing out image data

This example illustrates how to use `GridFormat` to write out data that is defined on a structured, two-dimensional and equi-spaced
grid (e.g. an image). A representative use case could be some simulation code that produces a numerical solution on an image grid
and stores it in a `std::vector<double>`. In this example code, we demonstrate how one can use `GridFormat` to write out such data
into established file formats, in this case, the VTI file format. In order to do so, we create a small custom data structure that
represents the image grid, and specialize the traits required for the [ImageGrid concept](../../docs/pages/grid_concepts.md#image-grid).
Note that we could also use the predefined `GridFormat::ImageGrid` instead of defining a new data structure, but here we want to illustrate
on a very simple example how the traits specialization can be done.

What you can see in this example:

- how to specialize the traits required for the `ImageGrid` concept.
- how to statically check if your traits specializations are correct.
- how to let `GridFormat` choose a default file format for your grid.
- how to write out discrete values as cell data.

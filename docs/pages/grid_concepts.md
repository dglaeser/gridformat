<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Grid Concepts

This document is meant to give an overview over the kinds of grids that are supported in `GridFormat`.
The distinctions used here are inspired by the different [VTK-XML formats](https://examples.vtk.org/site/VTKFileFormats/#serial-xml-file-formats)
and the kinds of grids they assume. Any grid can, in principle, be represented as an unstructured grid, but there exist file
formats that are able to store the information more efficiently in case the grid has certain characteristics.


## Unstructured Grid

<img alt="unstructured grid" src="https://raw.githubusercontent.com/dglaeser/gridformat/main/docs/img/grid_unstructured.svg" width="70%"/>

This is the most general type of grid representation. It consists of a list of points and an arbitrary number of cells of
possibly different geometry types, which are arbitrarly arranged. Therefore, one has to explicitly define the geometry type of
each cell and which corners make up its boundary. The corners have to also be given in a specific order, and in `GridFormat` we
use the [VTK ordering (scroll down to figures 2/3)](https://examples.vtk.org/site/VTKFileFormats/#legacy-file-examples).

To satisfy the `GridFormat::Concepts::UnstructuredGrid` concept, a grid implementation needs to specialize
the traits as specified in @ref mandatory-traits and @ref traits-for-unstructured-grids.

## Structured Grid

<img alt="structured grid" src="https://raw.githubusercontent.com/dglaeser/gridformat/main/docs/img/grid_structured.svg" width="40%"/>

In a structured grid, all cells are of the same geometry type, namely line segments in 1D, quadrilaterals in 2D and hexahedra in 3D.
A structured grid can be described as a set of $(N_x \times N_y \times N_z )$ cells and $(N_x + 1 \times N_y + 1 \times N_z + 1 )$
points (in 3D), and one can associate an index tuple $(i_x, i_y, i_z)$ to each point or cell, which describes its location within
the index space of the grid. The actual coordinates of the points can be chosen arbitrarily to the extent that they still describe
valid, non self-intersecting hexahedra (or quadrilaterals in 2D; segments in 1D).

To satisfy the `GridFormat::Concepts::StructuredGrid` concept, a grid implementation needs to specialize
the traits as specified in @ref mandatory-traits and @ref traits-for-structured-grids.


## Rectilinear Grid

<img alt="rectilinear grid" src="https://raw.githubusercontent.com/dglaeser/gridformat/main/docs/img/grid_rectilinear.svg" width="35%"/>

A rectilinear grid has the same arrangement of cells as a structured grid, but it additionally assumes that the faces of the cells
are aligned with the coordinate axes. That is, the grid consists of line segments in 1D, axis-aligned rectangles in 2D and
axis-aligned boxes. Along one coordinate direction, the size of the cells can change arbitrarily from one cell to another. In
contrast to the structured grid, it is not necessary to specify the coordinates of all points of the grid, instead it is sufficient
to specify only the coordinates along the axes.

To satisfy the `GridFormat::Concepts::RectilinearGrid` concept, a grid implementation needs to specialize
the traits as specified in @ref mandatory-traits and @ref traits-for-rectilinear-grids.


## Image Grid

<img alt="image grid" src="https://raw.githubusercontent.com/dglaeser/gridformat/main/docs/img/grid_image.svg" width="38%"/>

An image grid has again the same arrangement of cells as the structured and rectilinear grids, yet it additionally assumes that the
size of the cells does not change along a coordinate direction. Thus, in this case it suffices to store the size of the domain in each coordinate direction and the number of cells used per direction to discretize the domain. From these numbers the topology of
the entire grid can be reconstructed.

To satisfy the `GridFormat::Concepts::ImageGrid` concept, a grid implementation needs to specialize
the traits as specified in @ref mandatory-traits and @ref traits-for-image-grids.

<!-- DOXYGEN_ONLY [TOC] -->

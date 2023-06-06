<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 4: using the predefined traits for [`Dune::GridView`](https://www.dune-project.org/)

`Dune` users can use `GridFormat` out-of-the box. Simply include the header with the predefined traits,
and then you can use all provided writers for your `Dune::GridView`. For `Dune::GridView`, all traits
required for the unstructured grid concept are specialized. However, grid views of `Dune::YaspGrid` can
be used to write file formats for
[`ImageGrid`s](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#image-grid)
or [`RectilinearGrid`s](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#rectilinear-grid),
depending on the variant of `Dune::YaspGrid` you use.

In this example you can see:

- how to use the predefined traits for `Dune::GridView`
- how to let `GridFormat` select a suitable file format for your `GridView`
- a possible way how to write out discrete numerical data from simulations
- write discontinuous output by using the provided `DiscontinuousGrid` wrapper

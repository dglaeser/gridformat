<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Example 4: using the predefined traits for [`Dune::GridView`](https://www.dune-project.org/)

`Dune` users can use `GridFormat` out-of-the box. Simply include the header with the predefined traits,
and use all provided writers for your `Dune::GridView`. For a general `Dune::GridView`, all traits
required for the unstructured grid concept are specialized. However, grid views of `Dune::YaspGrid` can
be used to write file formats for
[`ImageGrid`s](../../docs/pages/grid_concepts.md#image-grid)
or [`RectilinearGrid`s](../../docs/pages/grid_concepts.md#rectilinear-grid),
depending on the variant of `Dune::YaspGrid` you are using.

In this example you can see:

- how to use the predefined traits for `Dune::GridView`
- how to let `GridFormat` select a suitable file format for your `GridView`
- a possible way how to write out discrete numerical data from simulations
- write discontinuous output by using the provided `DiscontinuousGrid` wrapper

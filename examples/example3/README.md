<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 3: using the predefined [CGAL](https://www.cgal.org/) traits

`GridFormat` comes with support for `CGAL` triangulation out-of-the-box. The only thing that's required is to include the
header with the predefined traits, and provide an implementation for the missing `PointId` trait. `CGAL` does not have a standard
way of indexing vertices of a triangulation, but for the [`UnstructuredGrid`](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#unstructured-grid)
concept, it is required that a unique id can be retrieved for each vertex in order to determine the connectivity of the grid.
Without a general and robust way of determining vertex ids, this missing trait has to be specialized by the users. In this example
we use the common way of attaching an `Info` object to vertices, in which we store its index.

This examples shows:

- how to specialize the `PointId` trait
- use chaining (via the `with_*` syntax) to conveniently set the options on a file format (all VTK-XML formats support this)

<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Using the predefined [CGAL](https://www.cgal.org/) traits

`GridFormat` comes with support for `CGAL` triangulations out-of-the-box. An important aspect of the predefined
`CGAL` traits is that they use `Triangulation::Vertex_handle` and `Triangulation::Face_handle` (2D) or `Triangulation::Cell_handle` (3D) as
point and cell types. Using the `CGAL` handles is more flexible than using the dereferenced types directly, but it forces users to use
pointer semantics in the field setter functions. For instance:

```cpp
CGAL::Triangulation_2 triangulation;
// ...
GridFormat::Writer writer{GridFormat::vtu, triangulation};
writer.set_point_field("x_coordinate", [] (const auto& vertex_handle) {
    // return vertex_handle.point().x();  // This does not work since this is a handle!
    return vertex_handle->point.x();
});
```

This examples shows:

- how to use the predefined `CGAL` traits
- use chaining (via the `with_*` syntax) to conveniently set the options on a file format (all VTK-XML formats support this)

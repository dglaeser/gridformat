<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Example 5: using the predefined traits for [dolfinx meshes and functions](https://github.com/FEniCS/dolfinx)

Users of `dolfinx` can use `GridFormat` to write out their meshes and functions. `dolfinx` has support for a
variety of file formats, however, with `GridFormat` you can additionally use different encoders (besides ascii)
and compression with the VTK-XML file formats, or the VTK-HDF file format. For `dolfinx::mesh::Mesh` all traits
required for the [`UnstructuredGrid`](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md#unstructured-grid)
concept are specialized. For `dolfinx::fem::Function`, you have to use a predefined adapter around your function,
for which all the necessary traits are specialized.

In this example you can see:

- how to use the predefined traits for `dolfinx::mesh::Mesh`
- how to use the predefined adapter for `dolfinx::fem::Function` to get access to the `GridFormat` writers
- how to write higher-order nodal functions to the `.vtu` file format

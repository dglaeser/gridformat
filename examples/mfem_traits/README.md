<!-- SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Using the predefined traits for [mfem::Mesh](https://github.com/mfem/mfem)

Users of `mfem` can use `GridFormat` to write out their meshes and functions.
`mfem` has some native support for VTK formats, however, with `GridFormat` you can additionally use different encoders
and compression with the VTK-XML file formats, or, use the VTK-HDF file format. For `mfem::Mesh` all traits
required for the [`UnstructuredGrid`](../../docs/pages/grid_concepts.md#unstructured-grid)
concept are specialized.

In this example you can see:

- how to use the predefined traits for `mfem::Mesh`
- how to write nodal functions to the `.vtu` file format

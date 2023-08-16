<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# `GridFormat` Examples

This folder contains several examples, each of which is contained in a separate sub-folder including a more detailed description.
Each of the examples can be run by heading into the respective subfolder, configuring the example with

```bash
# if your default compiler is compatible, you don't need to set the compiler paths
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
      -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
      -DCMAKE_BUILD_TYPE=Release \
      -B build
```

and then going into the `build` directory for compilation and execution:

```bash
# Substitute ${EXAMPLE} with the name of the example / folder
cd build && make && ./${EXAMPLE}
```

The examples also include how to use some of the predefined traits for various frameworks. Even if you don't plan to
use those frameworks, you may still want to have a quick look at those examples as they may demonstrate features you'd
be interested in.

- [analytical](./analytical): write out discretized analytical expressions.
- [minimal](./minimal): a minimal example illustrating how to specialize the traits for a custom data structure.
- [voxels](./voxels): register a custom data structure of voxels as `ImageGrid` without defining grid points.
- [dem](./dem): register a data structure representing a digital elevation model (dem) as `StructuredGrid`
- [parallel](./parallel): reading/writing grid files in parallel computations using [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface).
- [cgal_traits](./cgal_traits): using the predefined traits for [CGAL](https://www.cgal.org/).
- [dune_traits](./dune_traits): using the predefined traits for [Dune::GridView](https://dune-project.org/).
- [dolfinx_traits](./dolfinx_traits): using the predefined traits for [dolfinx](https://github.com/FEniCS/dolfinx).
- [mfem_traits](./mfem_traits): using the predefined traits for [MFEM](https://mfem.org/).

<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

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
# N should be substituted with 1, 2, ..., depending on the example you want to run
cd build && make && ./exampleN
```

The examples also include how to use some of the predefined traits for various frameworks. Even if you don't plan to
use those frameworks, you may still want to have a quick look at those examples as they may demonstrate features you'd
be interested in.

- [Example 1](./example1): register a custom data structure as `ImageGrid`
- [Example 2](./example2): register a custom data structure as `StructuredGrid`
- [Example 3](./example3): using the predefined traits for [CGAL](https://www.cgal.org/).
- [Example 4](./example4): using the predefined traits for [Dune::GridView](https://dune-project.org/).
- [Example 5](./example5): using the predefined traits for [dolfinx](https://github.com/FEniCS/dolfinx).

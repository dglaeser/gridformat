<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# `GridFormat` Examples

This folder contains several examples, each of which is contained in a separate sub-folder including a more detailed description.
Each of the examples can be run by heading into the respective subfolder, configuring the example with

```bash
# if your default compiler is compatible, you don't need to set the paths
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
      -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
      -B build
```

and then going into the `build` directory for compilation and execution:

```bash
# N should be substituted with 1, 2, ..., depending on the example you want to run
cd build && make && ./exampleN
```

- [Example 1](./example1): writing a custom image data structure into `.vti` files
- [Example 2](./example2): writing a digital elevation model into a `.vts` file

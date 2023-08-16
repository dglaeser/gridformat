<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Using the predefined traits for [`Dune::GridView`](https://www.dune-project.org/)

`Dune` users can use `GridFormat` out-of-the box. Simply include the header with the predefined traits,
and use all provided writers for your `Dune::GridView`. For a general `Dune::GridView`, all traits
required for the unstructured grid concept are specialized. However, grid views of `Dune::YaspGrid` can
be used to write file formats for
[`ImageGrid`s](../../docs/pages/grid_concepts.md#image-grid)
or [`RectilinearGrid`s](../../docs/pages/grid_concepts.md#rectilinear-grid),
depending on the variant of `Dune::YaspGrid` you are using.

Note that this example requires several `Dune` modules to be found on your system. You can use the convenience
script `install_dune.py` to clone and install those packages into a sub-folder `dune`. We recommend to use the
same compilers as you use for the compilation of the example:

```bash
python3 install_dune.py --c-compiler=/usr/bin/gcc-12 --cxx-compiler=/usr/bin/g++-12
```

Afterwards, you can pass the newly created `dune` folder as prefix path to cmake when you configure this example:

```bash
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
      -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
      -DCMAKE_PREFIX_PATH=dune \
      -DCMAKE_BUILD_TYPE=Release \
      -B build
```

In this example you can see:

- how to use the predefined traits for `Dune::GridView`
- how to let `GridFormat` select a suitable file format for your `GridView`
- a possible way how to write out discrete numerical data from simulations
- write discontinuous output by using the provided `DiscontinuousGrid` wrapper
- how to write [dune-functions](https://gitlab.dune-project.org/staging/dune-functions)
into meshes composed of higher-order Lagrange cells by using the `GridFormat::Dune::LagrangePolynomialGrid` wrapper.

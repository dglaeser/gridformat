<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# GridFormat

[![Coverage Report.](https://dglaeser.github.io/gridformat/coverage.svg)](https://dglaeser.github.io/gridformat)

`GridFormat` is a header-only C++ library for writing data into mesh file formats that can be visualized with tools
such as [ParaView](https://www.paraview.org/). The typical use case for `GridFormat` is within codes for numerical simulations
that want to export numerical results into visualizable file formats. A variety of simulation frameworks exist, such as
[Dune](https://www.dune-project.org/), [DuMuX](https://dumux.org/), [Deal.II](https://www.dealii.org/) or [Fenics](https://fenicsproject.org/),
which usually provide mechanisms to export data produced with the framework into some file formats. However, there are situations
in which one wants to use a format that the framework does not support, or, use some features of the format specification that are
not implemented in the framework. `GridFormat` aims to provide access to a variety of file formats through a unified interface and
without the need to convert any data or to use a specific data structure to represent computational grids. Using generic programming
and traits classes, `GridFormat` fully operates on the user-given data structures, thereby minimizing the runtime overhead.
`GridFormat` also supports writing files from parallel computations that use [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface).


## Quick Start

Prerequisites:

- C++-compiler with C++-20-support (tests run with `gcc-12/13`)
- `cmake` (tests run with cmake-3.26)

The easiest way to integrate `GridFormat` is via the `FetchContent` module of `cmake`. A minimal example of a project using
`GridFormat` to write a [VTU](https://examples.vtk.org/site/VTKFileFormats/#unstructuredgrid) file may look like this:

```cmake
cmake_minimum_required(VERSION 3.26)
project(some_app_using_gridformat)

include(FetchContent)
FetchContent_Declare(
    gridformat
    GIT_REPOSITORY https://github.com/dglaeser/gridformat
    GIT_TAG main
    GIT_PROGRESS true
    GIT_SHALLOW true
    GIT_SUBMODULES_RECURSE OFF
)
FetchContent_MakeAvailable(gridformat)

add_executable(my_app my_app.cpp)
target_link_libraries(my_app PRIVATE gridformat::gridformat)
```

```cpp
#include <array>
#include <gridformat/gridformat.hpp>

double analytical_function(const std::array<double, 2>& position) {
    return position[0]*position[1];
}

int main () {
    // For this example we have no user-defined grid type. Let's just use a predefined one...
    GridFormat::ImageGrid<2, double> grid{
        {1.0, 1.0}, // domain size
        {10, 12}    // number of cells (pixels) in each direction
    };

    // Let's write an analytical field into .vtu file format. Fields are attached to the
    // writer via lambdas that return the discrete values at points or cells.
    GridFormat::Writer writer{GridFormat::vtu, grid};
    writer.set_point_field("point_field", [&] (const auto& point) {
        return analytical_function(grid.position(point));
    });
    writer.set_cell_field("cell_field", [&] (const auto& cell) {
        return analytical_function(grid.center(cell));
    });
    writer.write("my_test_file");  // the file extension will be appended by the writer

    return 0;
}
```

Many more formats and options are available, see the overview over supported formats (coming soon).
For more examples, have a look at the [examples folder](https://github.com/dglaeser/gridformat/tree/main/examples).


## Installation

To install `GridFormat` into a custom location, clone the repository, enter the folder and type

```bash
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
      -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
      -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
      -B build
cmake --install build
```

Note that you can omit the explicit definition of `C_COMPILER` and `CXX_COMPILER` in case your default compiler is compatible.
Moreover, for a system-wide installation you may omit the definition of `CMAKE_INSTALL_PREFIX`. After installation, you can
use `cmake` to link against `GridFormat` in your own project:

```cmake
find_package(gridformat)
target_link_libraries(... gridformat::gridformat)
```


## Compatibility with user-defined grids

`GridFormat` does not operate on a specific grid data structure, but instead, it can be made compatible with any user-defined
grid types by implementing specializations of a few traits classes. For information on how to do this, please have a look at the
[traits classes overview (coming soon)](https://github.com/dglaeser/gridformat/blob/main/docs/traits_overview.md)
and the guide on [how to make your grid compatible with `GridFormat` (coming soon)](https://github.com/dglaeser/gridformat/blob/main/docs/how_to.md).


## Contribution Guidelines

Coming soon...

## License

`GridFormat` is licensed under the terms and conditions of the GNU General Public License (GPL) version 3 or - at your option -
any later version. The GPL can be [read online](https://www.gnu.org/licenses/gpl-3.0.en.html) or in the
[LICENSES/GPL-3.0-or-later.txt](https://github.com/dglaeser/gridformat/blob/main/LICENSES/GPL-3.0-or-later.txt) file.
See [LICENSES/GPL-3.0-or-later.txt](https://github.com/dglaeser/gridformat/blob/main/LICENSES/GPL-3.0-or-later.txt) for full copying permissions.

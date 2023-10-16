<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

[![C++ Standard](https://img.shields.io/badge/C++-20-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/std/the-standard)
[![Pages](https://github.com/dglaeser/gridformat/actions/workflows/pages.yml/badge.svg)](https://dglaeser.github.io/gridformat)
[![Test suite](https://github.com/dglaeser/gridformat/actions/workflows/main.yml/badge.svg)](https://github.com/dglaeser/gridformat/actions?query=branch%3Amain+)
[![Coverage Report](https://dglaeser.github.io/gridformat/coverage.svg)](https://dglaeser.github.io/gridformat/test_coverage)
[![API Documentation](https://img.shields.io/badge/doc-API-ff69b4)](https://dglaeser.github.io/gridformat)
[![REUSE status](https://api.reuse.software/badge/git.fsfe.org/reuse/api)](https://api.reuse.software/info/git.fsfe.org/reuse/api)
[![JOSS](https://joss.theoj.org/papers/10.21105/joss.05778/status.svg)](https://doi.org/10.21105/joss.05778)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10008061.svg)](https://doi.org/10.5281/zenodo.10008061)

`GridFormat` is a header-only C++ library for reading/writing data from/to standardized grid file formats that are supported by visualization tools such as e.g. [ParaView](https://www.paraview.org/).
Thus, applications that operate on grid-like data structures such as numerical simulations, GIS or computational geometry applications, can leverage `GridFormat` to import/export their data from/into interoperable file formats.
The typical use case for `GridFormat` is within codes for numerical simulations, for visualization of results or for importing
them for further processing.
A variety of simulation frameworks exist, such as
[Dune](https://www.dune-project.org/), [DuMuX](https://dumux.org/), [Deal.II](https://www.dealii.org/), [Fenics](https://fenicsproject.org/)
or [MFEM](https://mfem.org/),
which usually provide mechanisms to export/import data produced with the framework into some file formats. However, there are situations
in which one wants to use a format that the framework does not support, or, use some features of the format specification that are
not implemented in the framework. `GridFormat` aims to provide access to a variety of file formats through a unified interface and
without the need to convert any data or to use a specific data structure to represent computational grids. Using generic programming
and traits classes, `GridFormat` fully operates on the user-given data structures, thereby minimizing the runtime overhead.
`GridFormat` also supports writing files from parallel computations that use [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface).
Ideally, simulation frameworks use `GridFormat` under-the-hood to avoid duplicate implementation efforts, and implement support for new
formats into `GridFormat` such that they are directly available to all other frameworks that utilize it.

Currently, `GridFormat` is focused on [VTK file formats](https://examples.vtk.org/site/VTKFileFormats/). However, the API is
suitable for any grid format describing one of the supported [grid concepts](docs/pages/grid_concepts.md). Contributions are welcomed,
see below for information on how to contribute.


## Quick Start

Prerequisites:

- C++-compiler with C++-20-support (tests run with `gcc-12/13`, `clang++-16`)
- `cmake` (tests run with cmake-3.26)

It is easiest to integrate `GridFormat` either as a [git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules)
or via the `FetchContent` module of `cmake`. A minimal example (using `FetchContent`) of a project using `GridFormat` to
write a [VTU](https://examples.vtk.org/site/VTKFileFormats/#unstructuredgrid) file and read it back in may look like this:

```cmake
cmake_minimum_required(VERSION 3.22)
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
#include <vector>
#include <gridformat/gridformat.hpp>

double f(const std::array<double, 2>& x) { return x[0]*x[1]; }

int main () {
    // For this example we have no user-defined grid type. Let's just use a predefined one...
    GridFormat::ImageGrid<2, double> grid{
        {1.0, 1.0}, // domain size
        {10, 12}    // number of cells (pixels) in each direction
    };

    // This shows the `GridFormat` API: Construct a writer for the desired format, and add
    // point/cell fields as lambdas. Metadata can be added directly and can be ranges or scalars.
    // If the lambdas are not suitable or inefficient for your data structures, you can also pass
    // a (custom) implementation of `GridFormat::Field` (see documentation)
    GridFormat::Writer writer{GridFormat::vtu, grid};
    writer.set_meta_data("some_metadata", "i am metadata");
    writer.set_point_field("point_field", [&] (const auto& point) { return f(grid.position(point)); });
    writer.set_cell_field("cell_field", [&] (const auto& cell) { return f(grid.center(cell)); });
    const auto written_file = writer.write("my_test_file"); // extension is added by the writer

    // read the data back in (here we create a generic reader, but you can also select specific ones)
    GridFormat::Reader reader; reader.open(written_file);
    std::vector<double> cell_field_values(reader.number_of_cells());
    std::vector<double> point_field_values(reader.number_of_points());
    reader.cell_field("cell_field")->export_to(cell_field_values);
    reader.point_field("point_field")->export_to(point_field_values);

    return 0;
}
```
Many more formats, options and functions are available, see the [API documentation](https://dglaeser.github.io/gridformat/)
or have a look at the <!-- DOXYGEN_MAKE_ABSOLUTE -->[examples](examples).


## Installation

The recommended way of using `GridFormat` is to include it via cmake's `FetchContent` module (see quickstart).
However, if you want to install `GridFormat` locally into a custom location, clone the repository, enter the
folder and type

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

### Dependencies

`GridFormat` has no required dependencies, however, some features are only available if certain dependencies are present. For
instance, the [VTK-HDF file formats](https://examples.vtk.org/site/VTKFileFormats/#hdf-file-formats) are only available if
[`HighFive`](https://github.com/BlueBrain/HighFive) is found, which itself requires `libhdf5-dev`. If the latter is found on
your system, including `GridFormat` via cmake's `FetchContent` (see quickstart) automatically brings in `HighFive`, as it is
included in `GridFormat` as a git submodule. However, when installing `GridFormat` from the cloned sources (as described above),
make sure to use `git clone --recursive` in case you want to use the HDF file formats.

The availability of some specific features of a file format may also depend on the availability of certain dependencies. For
instance, compression of data (e.g. for the VTK-XML file formats) can only be used if the respective compression libraries are
found on the system. Dependencies of those features are stated in the [API documentation](https://dglaeser.github.io/gridformat/).


## Command-line interface

`GridFormat` comes with a few command-line apps that you can build and install alongside the library.
To include them in the build, pass the option `-DGRIDFORMAT_BUILD_BINARIES=ON` to `cmake` when configuring (see above).
For performance reasons, you should set `-DCMAKE_BUILD_TYPE=Release` when configuring. If successfully built, you can then
use the command-line apps to print information on a grid file to the terminal, or convert between different file formats.
For instance:

```cpp
gridformat-info my_vti_file.vti        # prints info on the contents of the vti file
gridformat-convert my_vti_file.vti vtu # converts an image grid format (.vti) to vtu format
gridformat-convert my_vti_file.vti vtu encoder=ascii # format options can be set as key-value pairs
gridformat-convert my_vti_file.vti vtu -o some_file  # choose an output filename
```


## Compatibility with user-defined grids

`GridFormat` does not operate on a specific grid data structure, but instead, it can be made compatible with any user-defined
grid types by implementing specializations for a few traits classes. For information on how to do this, please have a look at the
[traits classes overview](docs/pages/traits.md),
the <!-- DOXYGEN_MAKE_ABSOLUTE -->[examples](examples),
the predefined <!-- DOXYGEN_MAKE_ABSOLUTE -->[image grid implementation](gridformat/grid/image_grid.hpp)
or the predefined <!-- DOXYGEN_MAKE_ABSOLUTE -->[traits for several frameworks](gridformat/traits).


## Predefined traits

`GridFormat` comes with predefined traits for
[dune grid views](https://www.dune-project.org/doxygen/2.8.0/classDune_1_1GridView.html) (tested dune version: 2.9),
[deal.ii triangulations](https://www.dealii.org/current/doxygen/deal.II/classTriangulation.html) (tested deal.ii version: 9.4.0),
[cgal](https://www.cgal.org/) triangulations in
[2d](https://doc.cgal.org/latest/Triangulation_2/index.html) and
[3d](https://doc.cgal.org/latest/Triangulation_3/index.html) (tested cgal version: 5.5.2),
[dolfinx](https://github.com/FEniCS/dolfinx) meshes and function spaces (tested dolfinx version: 0.6.0)
and [mfem](https://mfem.org/) meshes (tested mfem version: 4.5.2).
Users of these frameworks can include these predefined traits and use `GridFormat` directly
(see the <!-- DOXYGEN_MAKE_ABSOLUTE -->[examples](examples)).


## Caveats

When reading from grid files, `GridFormat` provides access to the data as specified by the file format. These specifications may not be
sufficient in all applications. For instance, to fully instantiate a simulator for parallel computations, information on the grid
entities shared by different processes is usually required. Since these requirements are simulator-specific, any further processing has
to be done manually by the user and for their data structures. The recommended way to deal with this issue is to add any information
required for reinstantiation as data fields to the output. This way, it is readily available when reading the file.


## Getting help

Find answered questions, ask questions
or start discussions through [GitHub Discussions](https://github.com/dglaeser/gridformat/discussions).


## Contribution Guidelines

Contributions are highly welcome! For bug reports, please file an [issue](https://github.com/dglaeser/gridformat/issues).
If you want to contribute with features, improvements or bug fixes please fork this project and open
a merge request into the main branch of this repository.

### Development and test suite

In order to configure your local copy for testing, tell `cmake` to include the test suite:

```bash
# Note: you may have to set a compiler explicitly (see installation section)
cmake -DGRIDFORMAT_BUILD_TESTS=ON -B build
```

Afterwards, you can build and run all tests with `ctest`:

```bash
# Note: use, e.g., ctest -j4 if you want to use 4 processors
cd build
make build_tests
ctest
```

Note that an internet connection is required for the call to `cmake` as it pulls in [ut](https://github.com/boost-ext/ut) on-the-fly.
Moreover, in the configure step a Python script is invoked that produces some test data using [VTK](https://pypi.org/project/vtk/).
If your Python environment does not have `VTK`, this step is skipped. Note that some tests in the test suite will be skipped in this case.

### Creating a release

To create a release, you may use the utility script `util/update_versions.py`, which creates a git tag and adjusts the versions and
release dates specified in the cmake setup and the `CITATION.cff` file. For each release we maintain a separate branch for bugfixes
and patch releases. As an example, to create a release version `1.2.3`, you may type the following into the console (assumes a clean
repository):

```bash
git switch --create releases/1.2.3
# ... maybe continue development ...
python3 util/update_versions.py -v 1.2.3 # modifies versions&dates and creates a commit + tag
git push origin releases/1.2.3
git push origin v1.2.3
```

Afterwards, a release workflow will be triggered. If this runs through successfully, a release has been created. If not, the new tag
has to be deleted and the procedure has to be repeated after fixing the errors. After a successful release, the version on `main`
should be increased (without triggering an actual release). Following the above example, you may run the following commands:

```bash
git switch main
git switch --create feature/bump-version
python3 util/update_versions.py -v 1.2.4 --skip-tag # only modifies versions, no commit or tag
git commit -m "bump version to v1.2.4" .
git push origin feature/bump-version
```

and pose a pull request for the changes to be incorporated in `main`.


## License

`GridFormat` is licensed under the terms and conditions of the MIT License. It can be [read online](https://opensource.org/license/mit/)
or in the <!-- DOXYGEN_MAKE_ABSOLUTE -->[LICENSES/MIT.txt](LICENSES/MIT.txt) file.
See <!-- DOXYGEN_MAKE_ABSOLUTE -->[LICENSES/MIT.txt](LICENSES/MIT.txt) for full copying permissions.

---
title: 'GridFormat: header-only C++-library for grid file I/O'
tags:
  - C++
  - simulation
authors:
  - name: Dennis Gläser
    orcid: 0000-0001-9646-881X
    corresponding: true
    affiliation: "1"
  - name: Timo Koch
    orcid: 0000-0003-4776-5222
    affiliation: "2"
  - name: Bernd Flemisch
    orcid: 0000-0001-8188-620X
    affiliation: "1"
affiliations:
 - name: University of Stuttgart, Germany
   index: 1
 - name: University of Oslo, Norway
   index: 2
date: TODO
bibliography: paper.bib
---

# Summary

Numerical simulations play a crucial role in various research domains including mathematics, physics, and engineering.
Such simulations typically involve solving a set of model equations that describe the physical system under investigation.
To find an approximate solution to these equations on a given domain geometry and with specific boundary conditions, the
domain is usually discretized into a grid composed of points and cells, on which discretization schemes such as finite differences,
finite volumes, or finite elements are then employed. This process yields a discrete solution defined at specific grid positions,
which, depending on the scheme, can be interpolated over the entire domain using its basis functions. Due to the high computational
demand of such simulations, developers often implement simulation codes in performant C++ and leverage distributed-memory
parallelism through `MPI`, the _Message Passing Interface_ [@mpi_1994; @mpi_web], to run them on large high-performance computing
systems.

Visualization plays a fundamental role in analyzing numerical results, and one widely-used visualization tool in research is
`ParaView` [@ahrens2005_paraview; @paraview_web], which is based on `VTK`, the _Visualization Toolkit_ [@vtk_book; @vtk_web]. `ParaView` can read results from a wide range of
file formats, with the [VTK file formats](https://examples.vtk.org/site/VTKFileFormats/) being among the most popular.
To visualize simulation results with `ParaView`, researchers need to write their data into one of the supported file formats.
Users of existing simulation frameworks such as `Dune` [@bastian2008; @Dune2021],
`Dumux` [@dumux_2011; @Kochetaldumux2021], `Deal.II` [@dealII94], `FEniCS` [@fenicsbook2012; @fenics] or `MFEM` [@mfem; @mfem_web],
can usually export their results into some standard file formats, however, they are limited
to those formats that are supported by the framework. Reusing another framework's I/O functionality is generally challenging, at least
without runtime and memory overhead due to data conversions, since the implementation is typically tailored to its specific data structures.
As a consequence, the work of implementing I/O into standard file formats is currently repeated in every framework and remains inaccessible for
researchers developing new simulation frameworks or other research codes relying on I/O for visualization.

To address this issue, `GridFormat` aims to provide an easy-to-use and framework-agnostic API for reading from and writing to a variety of grid file formats.
By utilizing generic programming with C++ templates and traits, `GridFormat` is data-structure agnostic and allows developers to achieve
full interoperability with their data structures by implementing a small number of trait classes (see discussion below). Users of both
simulation frameworks and self-written small codes can write grid-based data into standard file formats with minimal effort and without significant
runtime or memory overhead. `GridFormat` comes with out-of-the-box support for data structures of several widely-used frameworks,
namely `Dune`, `Deal.II`, `FenicsX`, `MFEM`, and `CGAL` [@cgal; @cgal_web].

# Statement of Need

`GridFormat` addresses the issue of duplicate implementation effort for I/O across different simulation frameworks. By utilizing `GridFormat` as a backend for visualization file output, framework developers can easily provide their users with access to additional file formats. Moreover, instead of implementing support for new formats within the framework, developers can integrate them into `GridFormat`, thereby making them available to all other frameworks that use `GridFormat`. In addition to benefiting framework developers and users, the generic implementation of `GridFormat` also serves researchers with framework-independent smaller simulation codes.

Three key requirements govern the design of `GridFormat`: seamless integration, minimal runtime and memory overhead, and support for `MPI`. Given that C++ is widely used in grid-based simulation codes for performance reasons, we selected C++ as the programming language such that `GridFormat` can be adopted and used natively. It is lightweight, header-only, free of dependencies (unless opt-in features such as HDF5 output is desired), and supports CMake [@cmake_web] features that allow for automatic integration of `GridFormat` in downstream projects.

A comparable project in Python is `meshio` [@meshio], which supports reading from and writing to a wide range of grid file formats. However, accessing it from within simulators written in C++ would introduce an undesirable performance penalty, as well as memory overhead since `meshio` operates on an internal mesh representation that users have to convert their data into. `Dune` users can employ `dune-vtk` [@dune_vtk], which supports I/O for a number VTK-XML file formats and flavours, however, its implementation is strongly coupled to the `dune-grid` interface and can therefore not be easily reused in other contexts. To the best of our knowledge, a framework-independent solution that fulfills the above-mentioned requirements does not exist.

# Concept

Following the distinct [VTK-XML](https://examples.vtk.org/site/VTKFileFormats/#xml-file-formats) file formats, `GridFormat`
supports four different _grid concepts_: `ImageGrid`, `RectilinearGrid`, `StructuredGrid`, and `UnstructuredGrid`. While the
latter is fully generic, the first three assume that the grid has a structured topology. A known structured topology makes
it obsolete to define cell geometries and grid connectivity, and formats designed for such grids can therefore store the grid
in a space-efficient manner. An overview of the different types of grids is shown in the image below, and a more detailed
discussion can be found in the [`GridFormat` documentation](https://github.com/dglaeser/gridformat/blob/40596747e306fa6b899bdc5a19ae67e2308952f4/docs/pages/grid_concepts.md).

\begin{figure}[htb]
    \centering
    \includegraphics[width=.99\linewidth]{grids.pdf}
    \caption{Overview over the grid concepts supported in \texttt{GridFormat}. While \texttt{UnstructuredGrid}s are fully general, the first three have a structured topology.}
    \label{img:grids}
\end{figure}


`GridFormat` uses a traits (or meta-function) mechanism to operate on user-given grid types, and to identify which concept
a given grid models. As a motivating example, consider the following function template:

```cpp
template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : grid.cells()) {
        // ...
    }
}
```

In the function body, we iterate over all cells of the given grid by calling the `cells` method. This limits the usability
of this function to grid types that fulfill such an interface. One could wrap the grid in an adapter that exposes the required
interface method. However, this can become cumbersome, especially if there are certain requirements on the cell type
in the iterated range.
An alternative is to use traits, which allows writing the function generically, accepting any instance of a grid type that
the `Cells` trait class template is specialized for (by using (partial) template specialization):

```cpp
namespace Traits { template<typename Grid> struct Cells; }

template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : Traits::Cells<Grid>::get(grid)) {
        // ...
    }
}
```

Instead of calling a function on `grid` directly, it is accessed via `Cells`, which can be specialized for any type.
If such specialization exists, `do_something_on_a_grid` is invocable with an instance of type `Grid` directly, without the need for
wrappers or adapters. Using C++-20 concepts, `GridFormat` can check at compile-time if a user grid specializes all required traits correctly.
Error messages emitted by the compiler indicate which trait specializations are missing or incorrect.
The traits mechanism makes the `GridFormat` library fully extensible: users can achieve compatibility
with their concrete grid type by specializing the
required traits within _their_ code base, without having to change any code in `GridFormat`. Moreover, `GridFormat` comes with predefined
traits for `Dune`, `FenicsX`, `Deal.II`, `MFEM` and `CGAL` such that users of these frameworks can directly use `GridFormat` without
any implementation effort.

Note that each of the above-mentioned grid concepts requires the user to specialize a certain subset of traits.
For instance, to determine the
connectivity of an unstructured grid, `GridFormat` needs to know which points are embedded in a given grid cell.
The information is not required for writing structured grids into structured grid file formats.
An overview of which traits are required for which grid concept can be
found in the [`GridFormat` documentation](https://github.com/dglaeser/gridformat/blob/40596747e306fa6b899bdc5a19ae67e2308952f4/docs/pages/traits.md).


## Minimal Example

Let us assume that we have some hand-written code that performs simulations on a two-dimensional, structured grid
and yields a `std::vector<double>` storing the numerical solution. Due to this known trivial topology, the code does not
define a specific data structure to represent grids. It only needs to know the number of cells per direction and the size of one cell.
In the following code snippets, we demonstrate how one can use `GridFormat` to write such data into established file formats,
in this case, the [VTI](https://examples.vtk.org/site/VTKFileFormats/#imagedata) file format. Stitched together, the subsequent snippets
constitute a fully working C++ program.

First, let us define a simple data structure to represent the grid:

```cpp
#include <array>

struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};
```

We can specialize the traits required for the `ImageGrid` concept
for the type `MyGrid` (see the code comments for more details).

```cpp
// Include the GridFormat library header (brings in the traits declarations)
#include <gridformat/gridformat.hpp>

namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows iterating over all index tuples within the given
// dimensions (in our case the number of cells in each coordinate direction)
// MDIndexRange yields objects of type MDIndex, and thus, GridFormat deduces
// that MDIndex is the cell type of `MyGrid`.
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0]+1, grid.cells[1]+1}};
    };
};

// Expose the number of cells of the image grid per coordinate direction
template<> struct Extents<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.cells;
    }
};

// Expose the size of the cells per coordinate direction
template<> struct Spacing<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.dx;
    }
};

// Expose the position of the grid origin
template<> struct Origin<MyGrid> {
    static auto get(const MyGrid& grid) {
        // The origin for our grid's coordinate system is (0, 0)
        return std::array{0.0, 0.0};
    }
};

// For a given point or cell, expose its location (i.e. index tuple) within
// the structured grid arrangement. Our point/cell types are the same, i.e.
// GridFormat::MDIndex, since we used MDIndexRange in the Points/Cells traits.
template<> struct Location<MyGrid, GridFormat::MDIndex> {
    static auto get(const MyGrid& grid, const GridFormat::MDIndex& i) {
        return std::array{i.get(0), i.get(1)};
    }
};

}  // namespace GridFormat::Traits
```

With these traits defined, `MyGrid` can now be written out into file formats that are designed for image grids.
The following listing illustrates how to achieve this with the `GridFormat` API.

```cpp
int main() {
    std::size_t nx = 15, ny = 20;
    double dx = 0.1, dy = 0.2;

    // Here, there could be a call to our simulation code, but for this
    // simple example, let's just create a "solution vector" of ones.
    std::vector<double> values(nx*ny, 1.0);

    // To write out this solution, let's construct an instance of `MyGrid`
    // and construct a writer with it, letting GridFormat choose a format
    MyGrid grid{.cells = {nx, ny}, .dx = {dx, dy}};
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // We can now write out our numerical solution as a field on grid cells
    using GridFormat::MDIndex;
    writer.set_cell_field("cfield", [&] (const MDIndex& cell_location) {
        const auto flat_index = cell_location.get(1)*nx + cell_location.get(0);
        return values[flat_index];
    });

    // But we can also set an analytical function evaluated at cells/points
    writer.set_point_field("pfield", [&] (const MDIndex& point_location) {
        const double x = point_location.get(0)*dx;
        const double y = point_location.get(1)*dy;
        return x*y;
    });

    // GridFormat adds the extension to the provided filename
    const auto written_filename = writer.write("example");

    // The reader class allows you to read the data in the file back in.
    // See the documentation for details.
    GridFormat::Reader reader;
    reader.open(written_filename);
    reader.cell_field("cfield")->export_to(values);

    return 0;
}
```

# Acknowledgements

The authors would like to thank the Federal Government and the Heads of Government of the Länder, as well as the Joint Science
Conference (GWK), for their funding and support within the framework of the NFDI4Ing consortium. Funded by the German Research
Foundation (DFG) - project number 442146713.

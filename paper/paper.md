---
title: 'GridFormat: header-only C++-library for writing grid files'
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
parallelism through MPI (Message Passing Interface) [TODO:cite] to run them on large high-performance computing systems.

Visualization plays a fundamental role in analyzing numerical results, and one widely-used visualization tool in research is
[ParaView](TODO:CITE), which is based on the [Visualization Toolkit](TODO:CITE) (VTK). ParaView can read results from a wide range of
file formats, with the [VTK file formats](https://examples.vtk.org/site/VTKFileFormats/) being among the most popular.
To visualize simulation results with ParaView, researchers need to write their data into one of the supported file formats.
Users of existing simulation frameworks such as `Dune` [@bastian2008; @Dune2021],
`Dumux` [@dumux_2011; @Kochetaldumux2021], `Deal.II` [@dealII94], `FEniCS` [@fenicsbook2012; @fenics] or `MFEM` [TODO],
can usually export their results into some standard file formats, however, they are limited
to those formats that are supported by framework. Reusing another framework's I/O functionality is generally challenging, at least
without runtime and memory overhead due to data conversions, since the implementation is typically tailored to its specific data structures.
As a consequence, the work of implementing I/O into standard file formats is repeated in every framework, and remains inaccessible for
researchers that use hand-written simulation codes.

To address this issue, `GridFormat` aims to provide an easy-to-use API for writing grid files in various formats. By utilizing generic
programming through C++ templates and traits classes, `GridFormat` is designed to be data-structure agnostic and allows users to achieve
full interoperability with their data structures by implementing a few traits classes (see discussion below). This enables users of both
simulation frameworks and hand-written codes to write their data into standard file formats with minimal effort and without significant
runtime or memory overhead. In fact, `GridFormat` comes with out-of-the-box support for data structures of several widely-used frameworks,
namely `Dune`, `Deal.II`, `FenicsX`, `MFEM` and [CGAL](TODO:LINK).

# Statement of Need

`GridFormat` addresses the issue of duplicate implementation efforts for I/O across different simulation frameworks. By utilizing `GridFormat` as an underlying tool, framework developers can easily provide their users with access to additional file formats. Moreover, instead of implementing support for new formats within the framework, developers can integrate them into `GridFormat`, thereby making them available to all other frameworks that utilize it. In addition to benefiting framework developers and users, the generic implementation of `GridFormat` also serves researchers with custom simulation codes.

Three key requirements governed the design of `GridFormat`: seamless integration, minimal runtime and memory overhead, and support for MPI. Given that C++ is widely used in simulation codes, we selected it as the programming language such that `GridFormat` can be used natively. It is lightweight, header-only, free of mandatory dependencies, and supports [CMake](TODO:CITE) features that allow for automatic integration of `GridFormat` in downstream projects.

# Concept

Following the distinct [VTK-XML](https://examples.vtk.org/site/VTKFileFormats/#xml-file-formats) file formats, `GridFormat`
supports four different _grid concepts_: `ImageGrid`, `RectilinearGrid`, `StructuredGrid` and `UnstructuredGrid`. While the
latter is fully generic, the first three assume that the grid has a structured topology. A known structured topology makes
it obsolete to define cell geometries and grid connectivity, and formats designed for such grids can therefore store the grid
in a space-efficient manner. An overview over the different types of grids is shown in the image below, and a more detailed
discussion can be found in the `GridFormat` [repository](TODO: link).

\begin{figure}[htb]
    \centering
    \includegraphics[width=.99\linewidth]{grids.pdf}
    \caption{Overview over the grid concepts supported in \texttt{GridFormat}. While \texttt{UnstructuredGrid}s are fully general, the first three have a structured topology.}
    \label{img:grids}
\end{figure}


`GridFormat` uses a traits (or meta-function) mechanism to operate on user-given grid types, and to identify which concept
a given grid models. As a motivating example, consider this piece of code:

```cpp
template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : grid.cells()) {
        // ...
    }
}
```

This function internally iterates over all cells of the given grid by calling the `cells` function on it. This limits the usability
of this function to grids that fulfill this interface. As a user, one could wrap the grid in an adapter that exposes the required
interface. However, this can become cumbersome, especially if there are certain requirements on the cells that the iteration
yields. An alternative is to use traits, which can allow us to write this function generically, accepting any grid that implements
the `Cells` trait:

```cpp
namespace Traits { template<typename Grid> struct Cells; }

template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : Traits::Cells<Grid>::get(grid)) {
        // ...
    }
}
```

Instead of calling a function on `grid` directly, it is accessed via the `Cells` trait, which can be specialized for any type.
If such specialization exists, one can call `do_something_on_a_grid` with an instance of type `Grid` directly, without having to wrap it
in an adapter. Using C++-20 concepts, `GridFormat` checks if a user grid implements the required traits correctly at compile-time,
and the error messages emitted by the compiler indicate which traits are missing or which traits are not implemented correctly.

The different above-mentioned grid concepts require the user to specialize different traits. As an example, in order to determine the
connectivity of an unstructured grid, `GridFormat` needs to know which points are embedded in a given grid cell. To this end,
users operating on unstructured grids can specialize a specific trait for that. However, this is not required for structured grids
and for writing them into structured grid file formats. An overview over which traits are required for which grid concept can be
found in the `GridFormat` [documentation](TODO:LINK). As mentioned, for an easy integration with existing and widely-used grid data
structures, `GridFormat` comes with predefined traits for `Dune`, `FenicsX`, `Deal.II`, `MFEM` and `CGAL`.


## Minimal Example

Let us assume that we have some hand-written code that performs simulations on a two-dimensional, structured arrangement
of cells and yields a `std::vector<double>` storing the numerical solution. Due to this known trivial topology, the code does not
define a specific data structure to represent grids. It only needs to know the number of cells per direction and the size of one cell.
In the following code snippets, we want demonstrate how one could use `GridFormat` to write that data into established file formats,
in this case the [VTI](TODO:link) file format. Stitched together, the subsequent snippets constitute a fully working C++ main file.

First of all, let us define a simple data structure to represent the grid by collecting the above-mentioned information in a `struct`:

```cpp
#include <array>

struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};
```

Now we can specialize the traits required for the `ImageGrid` concept for the type `MyGrid` (see the code comments for more details).

```cpp
// bring in the library, this also brings in the traits declarations
#include <gridformat/gridformat.hpp>

namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows to iterate over all index tuples within the given
// dimensions (in our case the number of cells in each coordinate direction)
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points.
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0]+1, grid.cells[1]+1}};
    };
};

// Expose the number of cells of our "image grid" per direction.
template<> struct Extents<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.cells;
    }
};

// Expose the size of the cells per direction.
template<> struct Spacing<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.dx;
    }
};

// Expose the position of the grid origin.
template<> struct Origin<MyGrid> {
    static auto get(const MyGrid& grid) {
        // our grid always starts at (0, 0)
        return std::array{0.0, 0.0};
    }
};

// For a given point or cell, expose its location (i.e. index tuple) within the
// structured grid arrangement. Our point/cell types are the same, namely
// GridFormat::MDIndex, because we used MDIndexRange in the Points/Cells traits.
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

    // Here, there could be a call to our simulation code:
    // std::vector<double> = solve_problem(nx, ny, dx, dy);
    // But for this example, let's just create a vector filled with 1.0 ...
    std::vector<double> values(nx*ny, 1.0);

    // To write out this solution, let's construct an instance of `MyGrid`
    MyGrid grid{.cells = {nx, ny}, .dx = {dx, dy}};

    // ... and construct a writer, letting GridFormat choose a format.
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // We can now write out our numerical solution as a field on grid cells:
    using GridFormat::MDIndex;
    writer.set_cell_field("cfield", [&] (const MDIndex& cell_location) {
        const auto flat_index = cell_location.get(1)*nx + cell_location.get(0);
        return values[flat_index];
    });

    // But we can also just set an analytical function evaluated at cells/points
    writer.set_point_field("pfield", [&] (const MDIndex& point_location) {
        const double x = point_location.get(0)*dx;
        const double y = point_location.get(1)*dy;
        return x*y;
    });

    writer.write("example"); // gridformat adds the extension
    return 0;
}
```

# Acknowledgements

The authors would like to thank the Federal Government and the Heads of Government of the Länder, as well as the Joint Science
Conference (GWK), for their funding and support within the framework of the NFDI4Ing consortium. Funded by the German Research
Foundation (DFG) - project number 442146713.

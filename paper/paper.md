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
  - name: Bernd Flemisch
    orcid: 0000-0001-8188-620X
    affiliation: "1"
affiliations:
 - name: University of Stuttgart, Germany
   index: 1
date: TODO
bibliography: paper.bib
---

# Summary

Numerical simulations play an important role in a wide range of research domains such as mathematics, physics or engineering.
At the core of such simulations typically lies a set of model equations that describe the physical system of interest.
The task of the simulator is to find a numerical, that is, an approximated solution to these equations on a given domain
geometry and for given boundary conditions. A wide-spread approach is to discretize the domain into a grid consisting of
points and cells, and to use discretization schemes such as finite-differences, finite-volumes or finite-elements. As a
result of this process, one obtains a discrete solution that is defined at certain positions of the grid, and depending
on the scheme, one can interpolate the solution over the entire domain using the scheme's basis functions. Due to the
high computational demands involved in such simulations, simulation codes are typically written in performant C++,
leveraging distributed-memory parallelism via [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface) to be
run on large [High-Performance-Computing](TODO:REF) systems.

Visualization of plays a fundamental role in the process of analyzing numerical results, and a visualization tool that is widely
used in research is [ParaView](https://www.paraview.org), which is based on the [Visualization Toolkit (VTK)](https://www.vtk.org).
[ParaView](https://www.paraview.org) can read and understand a large number of different file formats, with some of the most
popular ones being the [VTK file formats](https://examples.vtk.org/site/VTKFileFormats/). Researchers that want to visualize their
results with `ParaView` must write them into one of the supported file formats. A solution could be to rely on one of the existing
research software simulation frameworks, as for instance, `Dune` [@bastian2008; @Dune2021], `Dumux` [@dumux_2011; @Kochetaldumux2021],
`Deal.II` [@dealII94], `FEniCS` [@fenicsbook2012; @fenics], or `VirtualFluids` [@virtualfluids_2022]. These frameworks typically
provide their users with functionality to export results produced with the framework into some standard file formats. As is natural
for a framework, this functionality is typically implemented for framework-specific data structures and is therefore hard to reuse
in different contexts; at least without having to convert data at the cost of runtime and memory overhead. As a consequence, the work
of implementing I/O into standard file formats is repeated in every framework, and remains inaccessible for researchers that use
hand-written simulation codes.

To close this gap, `GridFormat` aims to provide access to a variety of grid file formats through an easy-to-use API. Making
use of generic programming via C++ templates and traits classes, `GridFormat` is data-structure-agnostic, thereby enabling
both users of frameworks or hand-written codes to write out their data without runtime or memory overhead.

# Statement of Need

The idea for this library emerged in the context of the simulation framework `Dumux`. `Dumux` is based on `Dune`, which enables
users to write results into the [VTU file format](https://examples.vtk.org/site/VTKFileFormats/#unstructuredgrid), one of the
`VTK` file formats. However, `Dune` does not support writing compressed `VTU` files, or to use the `VTI` format
when working with regulary-spaced structured grids. Both features would have proven useful in a variety of projects around
`Dumux`, in which reduced file sizes would have helped managing the produced data. Conversion of the data after writing, for
instance with `ParaView`, is unsatisfactory due to the overhead of having to parse the file and to hold the entire
grid once more in memory. Especially for large and massively parallel simulations this can be problematic.

Instead of adding the desired features into `Dumux`, we created `GridFormat` with the goal to provide a generic solution that
could be reused with any framework or any hand-written simulation code alike. The implementation was governed by three major
requirements:

- 1. easy integration
- 2. no significant runtime or memory overhead
- 3. [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface)-support

A natural choice for the programming language was C++, which is used by a large number of simulation codes. We opted for a
lightweight, header-only approach, without mandatory dependencies and with support for [CMake](TODO:CITE) features that allow for
automatic integration of `GridFormat` in downstream projects.

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
interface, however, this can become cumbersome, especially if there are certain requirements on the cells that this expression
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
If a specialization exists, one can call `do_something_on_a_grid` with an instance of type `Grid` directly, without having to wrap it
in an adapter. Using C++-20 concepts, `GridFormat` checks if a user grid implements the required traits correctly at compile-time,
and the error messages emitted by the compiler indicate which traits are missing or which traits are not implemented correctly.

The above-mentioned grid concepts require the user to specialize different traits. As an example, in order to determine the
grid connectivity of an unstructured grid, `GridFormat` needs to know which points are embedded in a given grid cell. To this end,
users operating on unstructured grids can specialize a specific trait for that. However, this is not required for structured grids
and for writing them into structured grid file formats. An overview over which traits are required for which grid concept can be
found in the `GridFormat` [documentation](TODO:LINK). For easy integration with existing and widely-used grid data structures,
`GridFormat` comes with predefined traits for `Dune`, `FenicsX`, `Deal.II` and `CGAL`.


## Minimal Example

Let us assume that we have some hand-written code that performs simulations on a two-dimensional, structured arrangement
of cells and yields a `std::vector<double>` storing the numerical solution. Due to this known trivial topology, the code does not
define a specific data structure to represent grids. All that is required is the number of cells per direction and the size of one cell.
In the following listings, we want demonstrate how one could use `GridFormat` to write that data into established file formats, in this
case the [VTI](TODO:link) file format. Stitched together, the snippets constitute a fully working C++ main file.

First of all, let us define a simple data structure to represent the grid:

```cpp
#include <array>

struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};
```

Now we can specialize the traits that are required for the `ImageGrid` concept (see the code comments for more details).

```cpp
// bring in the library, this also brings in the traits declarations
#include <gridformat/gridformat.hpp>

namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows iterating over all index tuples within the given dimensions
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points.
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
        // let's simply return a range over index pairs for the given grid dimensions
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
        return std::array{grid.dx[0], grid.dx[1]};
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
The following listing shows a `main` function

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

    // ... and construct a writer with it. Let GridFormat choose a suitable format.
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // ... we can now write out our numerical solution as a field defined on cells:
    writer.set_cell_field("cfield", [&] (const GridFormat::MDIndex& cell_location) {
        const auto flat_cell_index = cell_location.get(1)*nx + cell_location.get(0);
        return values[flat_cell_index];
    });

    // ... but we can also just set am analytical function evaluated at cells/points
    writer.set_point_field("pfield", [&] (const GridFormat::MDIndex& point_location) {
        const double x = point_location.get(0);
        const double y = point_location.get(1);
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

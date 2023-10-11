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
Users of existing simulation frameworks, such as `Dune` [@bastian2008; @Dune2021],
`Dumux` [@dumux_2011; @Kochetaldumux2021], `Deal.II` [@dealII94], `FEniCS` [@fenicsbook2012; @fenics] or `MFEM` [@mfem; @mfem_web],
can usually export their results into some standard file formats. However, they are limited
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

A comparable project in Python is `meshio` [@meshio], which supports reading from and writing to a wide range of grid file formats. However, accessing it from within simulators written in C++ would introduce an undesirable performance penalty, as well as memory overhead, since `meshio` operates on an internal mesh representation that users have to convert their data into. `Dune` users can employ `dune-vtk` [@dune_vtk], which supports I/O for a number VTK-XML file formats and flavours, however, its implementation is strongly coupled to the `dune-grid` interface and can therefore not be easily reused in other contexts. To the best of our knowledge, a framework-independent solution that fulfills the above-mentioned requirements does not exist.

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

The traits are required for writing out grids and associated data, and are not needed when using `GridFormat` to read data from grid
files. `GridFormat` provides access to the data as specified by the file format, however, these specifications may not be sufficient
in all applications. For instance, to fully instantiate a simulator for parallel computations, information on the grid entities shared
by different processes is usually required. Since these requirements are simulator-specific, any further processing has to be done
manually by the user and for their data structures. The recommended way to deal with this issue is to add any information required for
reinstantiation as data fields to the output. This way, it is readily available when reading the file. For information on how to use these
features, we refer to the [API documentation](https://dglaeser.github.io/gridformat/) and the [examples](https://github.com/dglaeser/gridformat/tree/main/examples).

# Acknowledgements

The authors would like to thank the Federal Government and the Heads of Government of the Länder, as well as the Joint Science
Conference (GWK), for their funding and support within the framework of the NFDI4Ing consortium. Funded by the German Research
Foundation (DFG) - project number 442146713. TK acknowledges funding from the European Union’s Horizon 2020
research and innovation programme under the Marie Skłodowska-Curie grant agreement No 801133.

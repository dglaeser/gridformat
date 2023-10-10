<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Grid Traits

`GridFormat` uses a traits (or meta-function) mechanism to operate on user-given grid types. As a motivating example, consider this piece of code:

```cpp
template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : grid.cells()) {
        // ...
    }
}
```

`do_something_on_a_grid` iterates over all cells of the grid, and you can imagine that it then goes on to compute some stuff. The
point is that with this way of writing the function, it is only compatible with grids that have a public `cells()` function which returns
something over which we can iterate. This function would thus be incompatible with user-defined grids that provide a different way
of iterating over the cells. One possibility would be to require users to write an adapter, but then they possibly also have to
adapt the cell type that the iterator yields, etc...

Instead, in `GridFormat`, any piece of code that operates on user grids is written in terms of traits, and the code above would turn into
something like:

```cpp
// forward declaration
namespace Traits { template<typename Grid> struct Cells; }

template<typename Grid>
void do_something_on_a_grid(const Grid& grid) {
    for (const auto& cell : Traits::Cells<Grid>::get(grid)) {
        // ...
    }
}
```

The code now expects that there exist a specialization of the `Cells` trait for the given grid. This allows users to specialize
that trait for their grid data structure, thereby making it compatible with `GridFormat`. See the code below for an idea about how
the traits specialization may look. For more details on this, please have a look at the resources referenced in
the <!-- DOXYGEN_MAKE_ABSOLUTE -->[main readme](../../README.md).

```cpp
#include <gridformat/grid/traits.hpp>

namespace MyLibrary {

class Grid {
 public:
    // ...
    // returns a range over the grid elements
    std::ranges::range auto elements() const { /* ... */ }
    // ...
};

} // namespace MyLibrary

namespace GridFormat::Traits {

template<>
struct Cells<MyLibrary::Grid> {
    static auto get(const MyLibrary::Grid& grid) {
        return grid.elements();
    }
}

}  // namespace GridFormat::Traits
```

As discussed in the
[overview over supported kinds of grids](grid_concepts.md),
`GridFormat` understands the notion of unstructured, structured, rectilinear or image grids. The reason for this is that some
file formats are designed for specific kinds of grids and can store the information on their topology in a space-efficient manner.
To see which format assumes which kind of grid, see the [API documentation](https://dglaeser.github.io/gridformat/).
In the code, there exist [concepts](https://en.cppreference.com/w/cpp/language/constraints) for each of these kinds of grids,
which essentially check if the required traits are correctly implemented. When implementing the traits for your grid type, it is
helpful to use these concepts in order to verify your traits implementations. For instance, you may use `static_asserts`:

```cpp

#include <gridformat/gridformat.hpp>

class MyGrid { /* ... */ };

// let's specialize the traits for MyGrid (MyGrid is an unstructured grid)
namespace GridFormat::Traits {
    // ...
}

// let's directly check if we did that correctly
// if this static_assert passes, we are ready to go
static_assert(GridFormat::Concepts::UnstructuredGrid<MyGrid>);
```

The remainder of this page discusses the different traits used by `GridFormat`, and which ones need to be specialized in order to
model a particular kind of grid. Note that all traits presented in the following are declared in the `namespace GridFormat::Traits`.
In case you want to use `GridFormat` in parallel computations, please make sure to also read the related section at the end of this page.


<!-- DOXYGEN_ONLY [TOC] -->

### Mandatory Traits

- `template<typename Grid> struct Cells;`

This trait exposes how one can iterate over the cells of a grid. Specializations must provide a static function `get(const Grid&)`
that returns a [forward range](https://en.cppreference.com/w/cpp/ranges/forward_range) of cells. `GridFormat` deduces the cell type
used by the `Grid` from this range. As an example, let's consider a grid implementation (`SomeUnstructuredGrid`) that identifies cells
solely by an index. In this case, the following would be a valid specialization, which yields `int` as cell type:

```cpp
namespace GridFormat::Traits {
template<>
struct Cells<SomeUnstructuredGrid> {
    static std::ranges::forward_range auto get(const SomeUnstructuredGrid& grid) {
        return std::views::iota(0, grid.number_of_cells());
    }
};
}  // namespace GridFormat::Traits
```

- `template<typename Grid> struct Points;`

This trait exposes how one can iterate over the points of a grid. Specializations must provide a static function `get(const Grid&)`
that returns a [forward range](https://en.cppreference.com/w/cpp/ranges/forward_range) of points. `GridFormat` deduces the point type
used by the `Grid` from this range. As an example, let's again consider a grid implementation (`SomeUnstructuredGrid`) that identifies
points solely by an index. As before, we can simply return an index range yielding `int` as point type (`GridFormat` supports the case
of cell & point types being the same):

```cpp
namespace GridFormat::Traits {
template<>
struct Points<SomeUnstructuredGrid> {
    static std::ranges::forward_range auto get(const SomeUnstructuredGrid& grid) {
        return std::views::iota(0, grid.number_of_points());
    }
};
}  // namespace GridFormat::Traits
```


In the following we will discuss the traits required for particular grid concepts. Some of these traits expose information on a
single cell or point of the grid, and therefore, have to be specialized for the grid __and__ its point or cell type. As discussed
above, `GridFormat` deduces these types from the `Cells` and `Points` traits. In the following, we will refer to these types as `Cell`
and `Point`.


### Traits for Unstructured Grids

- `template<typename Grid, typename Cell> struct CellPoints;`

This trait exposes how one can iterate over the points of an individual grid cell. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns a [range](https://en.cppreference.com/w/cpp/ranges/range) of points that
are contained within the given cell. Note that the [value_type](https://en.cppreference.com/w/cpp/ranges/iterator_t) of the
returned range must match the point type deduced from the `Points` trait. Moreover, point ordering is expected to follow the
conventions used by [VTK](https://examples.vtk.org/site/VTKFileFormats/). Following the above example, an implementation of this trait
could look like this:

```cpp
namespace GridFormat::Traits {
template<>  // the cell type is `int` in this case, see the Cells trait description
struct CellPoints<SomeUnstructuredGrid, int> {
    static const std::ranges::range auto& get(const SomeUnstructuredGrid& grid, int cell_index) {
        return grid.point_indices_of_cell(cell_index);
    }
};
}  // namespace GridFormat::Traits
```

- `template<typename Grid, typename Cell> struct CellType;`

This trait exposes the geometry type of an individual grid cell. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns an instance of
the <!-- DOXYGEN_MAKE_ABSOLUTE -->[CellType enum](../../gridformat/grid/cell_type.hpp).
For the above example,
the specialization of this traits could look like this:

```cpp
namespace GridFormat::Traits {
template<>  // the cell type is `int` in this case, see the Cells trait description
struct CellType<SomeUnstructuredGrid, int> {
    static std::ranges::range auto get(const SomeUnstructuredGrid& grid, int cell_index) {
        return GridFormat::CellType::tetrahedron; // SomeUnstructuredGrid always only uses tets
    }
};
}  // namespace GridFormat::Traits
```

- `template<typename Grid, typename Point> struct PointCoordinates;`

This trait exposes the coordinates of a grid point. Specializations must provide a static function `get(const Grid&, const Point&)`
that returns the coordinates of the given point as a [statically sized range](#optional-traits). From the size of this range,
`GridFormat` deduces the space dimension of the grid at compile-time. If your grid does not know the space dimension at compile-time,
you can simply return an `std::array<double, 3>` with zero padding. For the above example, a specialization of this trait
may look as follows:

```cpp
namespace GridFormat::Traits {
template<>  // the point type is `int` in this case, see the Points trait description
struct PointCoordinates<SomeUnstructuredGrid, int> {
    static GridFormat::Concepts::StaticallySizedRange auto get(const SomeUnstructuredGrid& grid, int point_index) {
        // SomeUnstructuredGrid always operates in 3d, but the type used for coordinates is not compatible with
        // the StaticallySizedRange concept. We could make it compatible by implementing the respective
        // trait, but let's just construct an std::array with the coordinates ...
        const auto& point = grid.get_point(point_index);
        return std::array{
            point.x,
            point.y,
            point.z
        };
    }
};
}  // namespace GridFormat::Traits
```

- `template<typename Grid, typename Point> struct PointId;`

This trait exposes a unique id for individual points of a grid. Specializations must provide a static function
`get(const Grid&, const Point&)` that returns a unique id (as integer value, e.g. `std::size_t`) for the given point.
For our example above, we could directly return the `Point`, since we chose the `Points` trait to simply return a
range over all point indices:

```cpp
namespace GridFormat::Traits {
template<>  // the point type is `int` in this case, see the Points trait description
struct PointId<SomeUnstructuredGrid, int> {
    static int get(const SomeUnstructuredGrid& grid, int point_index) {
        return point_index;
    }
};
}  // namespace GridFormat::Traits
```


### Traits for Structured Grids

In addition to the traits below, the `StructuredGrid` concept also requires that the `PointCoordinates` trait is implemented
(see above).

- `template<typename Grid> struct Extents;`

This trait exposes the number of cells of the structured grid in each coordinate direction. Specializations must provide
a static function `get(const Grid&)` that returns a
[statically sized range](#optional-traits), whose size is equal to the dimension of the grid. As an example, let's consider
a structured grid implementation `SomeStructuredGrid` that has a `num_cells(int direction)` function:

```cpp
namespace GridFormat::Traits {
template<>
struct Extents<SomeStructuredGrid> {
    static GridFormat::Concepts::StaticallySizedRange auto get(const SomeStructuredGrid& grid) {
        // Let's assume SomeStructuredGrid is always two-dimensional
        return std::array{
            grid.num_cells(0),
            grid.num_cells(1)
        };
    }
};
}  // namespace GridFormat::Traits
```

- `template<typename Grid, typename Entity> struct Location;`

This trait exposes the index tuple of a given entity within the structured grid. For a visualization and the assumptions on the
orientation, see the [overview over supported kinds of grids](grid_concepts.md).
This trait must be specialized for both `Cell` and `Point` (i.e. for two types of `Entity`), and they must provide a static function
`get(const Grid&, const Entity&)` that returns a
[statically sized range](#optional-traits)
whose elements are integer values (the indices of the entity) and whose size is equal to the dimension of the grid. As an example,
let's consider a grid implementation that has points and cells that carry information about their location within the grid. An
implementation of this trait could then look like this:

```cpp
namespace GridFormat::Traits {
// Let's assume the `Point` and `Cell` types of `SomeStructuredGrid` have the same interface
// for obtaining their location. Let's therefore simply leave this trait a template on `Entity`,
// so that we don't have to implement it twice - once for `Point` and once for `Cell`
template<typename Entity>
struct Location<SomeStructuredGrid, Entity> {
    static GridFormat::Concepts::StaticallySizedRange auto get(const SomeStructuredGrid& grid, const Entity& e) {
        // Let's assume SomeStructuredGrid is always two-dimensional
        return std::array{
            e.x_index,
            e.y_index
        };
    }
};
}  // namespace GridFormat::Traits
```


### Traits for Rectilinear Grids

In addition to the `Ordinates` trait below, the `RectilinearGrid` concept also requires that the `Extents` and `Location` traits
are implemented (see above).

- `template<typename Grid> struct Ordinates;`

This trait exposes the ordinates of a `RectilinearGrid` grid along the coordinate axes.
Specializations must provide a static function `get(const Grid&, unsigned int direction)` that returns a
[range](https://en.cppreference.com/w/cpp/ranges/range) over the ordinates along the axis specified by `direction` (`direction < dim`, where dim is the dimension of the grid). The size of the range must be equal to the number of cells + 1 in the given
direction. As an example, let's consider an implementation `SomeRectilinearGrid` with the following specialization for this trait:

```cpp
namespace GridFormat::Traits {
template<>
struct Ordinates<SomeRectilinearGrid> {
    static std::ranges::range auto get(const SomeRectilinearGrid& grid, unsigned int direction) {
        const auto num_points = grid.num_cells(direction) + 1;
        const auto dx = grid.cell_size(direction);
        std::vector<double> ordinates; ordinates.reserve(num_points);
        for (std::size_t i = 0; i < num_points; ++i)
            ordinates.push_back(i*dx);
        return ordinates;
    }
};
}  // namespace GridFormat::Traits
```


### Traits for Image Grids

In addition to the traits below, the `ImageGrid` concept also requires that the `Extents` and `Location` traits are
implemented (see above).

- `template<typename Grid> struct Origin;`

This trait exposes the position of the lower-left corner of the grid, that
is, the position of the point at index $(0, 0, 0)$ (for a visualization
see [here](grid_concepts.md)
Specializations must provide a static function `get(const Grid&)` that returns a
[statically sized range](#optional-traits),
whose size is equal to the dimension of the grid. An exemplary specialization of this trait could look like:

```cpp
namespace GridFormat::Traits {
template<>
struct Origin<SomeImageGrid> {
    static GridFormat::Concepts::StaticallySizedRange auto get(const SomeImageGrid& grid) {
        // Let's say SomeImageGrid always starts at (0, 0, 0)
        return std::array{0., 0., 0.};
    }
};
}  // namespace GridFormat::Traits
```


- `template<typename Grid> struct Spacing;`

This trait exposes the spacing between grid points along the axes (in other words, the size of the cells in each coordinate
direction). Specializations must provide a static function `get(const Grid&)` that returns a
[statically sized range](#optional-traits), whose size is equal to the dimension of the grid. As an example, a valid
specialization of this trait may be:

```cpp
namespace GridFormat::Traits {
template<>
struct Spacing<SomeImageGrid> {
    static GridFormat::Concepts::StaticallySizedRange auto get(const SomeImageGrid& grid) {
        // Let's say SomeImageGrid is always 3D
        return std::array{
            grid.cell_size(0),
            grid.cell_size(1),
            grid.cell_size(2)
        };
    }
};
}  // namespace GridFormat::Traits
```

### Optional Traits

- `template<typename Grid> struct NumberOfPoints;`

This trait exposes the number of points of a grid, and if not specialized, the number of points is deduced from the size of the
range obtained from `Points`. If your point range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&)` that returns the number of points as an integral value.

- `template<typename Grid> struct NumberOfCells;`

This trait exposes the number of cells of a grid, and if not specialized, the number of cells is deduced from the size of the
range obtained from `Cells`. If your cell range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&)` that returns the number of cells as an integral value.

- `template<typename Grid, typename Cell> struct NumberOfCellPoints;`

This trait exposes the number of points in a grid cell, and if not specialized, the number of points is deduced from the size of the
range obtained from `CellsPoints`. If that range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns the number of points in the cell as an integral value.

- `template<typename Grid> struct Basis;`

This trait can be specified for image grids in order to specify their orientation. Per default, an image grid is assumed to be
axis-aligned, that is, the default basis (in 3D) is

```cpp
const auto default_basis = std::array{
    std::array{1.0, 0.0, 0.0},
    std::array{0.0, 1.0, 0.0},
    std::array{0.0, 0.0, 1.0}
};
```

Specializing the `Basis` trait, you can implement the static function `get(const Grid&)` and return a custom basis. It is expected
that the return type from this function is a two-dimensional,
[statically-sized range](#optional-traits),
with the outer and inner dimensions being equal to the grid dimension.


- `template<typename T> struct StaticSize;`

`GridFormat` requires that the types returned from some traits model
the <!-- DOXYGEN_MAKE_ABSOLUTE -->[`StaticallySizedRange` concept](../../gridformat/common/concepts.hpp#L25).
Statically sized means that the size of the [range](https://en.cppreference.com/w/cpp/ranges/range) is known at compile time.
Per default, `GridFormat` accepts `std::array`, `std::span` (with non-dynamic extent), `T[N]` or anything that either has a
`constexpr static std::size_t size()` function or a member variable named `size` that can be evaluated at compile time.
If your type does not fulfill any of these requirements, but is in fact range with a size known at compile-time, you can also
specialize the <!-- DOXYGEN_MAKE_ABSOLUTE -->[`StaticSize` trait](../../gridformat/common/type_traits.hpp)
for the type you want to return. Alternatively, you can of course just convert your type into an `std::array` within the
trait that expects you to return a statically sized range. To give an example, let's consider a `Vector` class whose size
is known at compile-time, but does not fulfill the requirements for `GridFormat` to automatically identify it as a
`StaticallySizedRange`:

```cpp
template<int dim>
class Vector { /* ... */ };

namespace GridFormat::Traits {
template<int dim>
struct StaticSize<Vector<dim>> {
    static constexpr int value = dim;
};
}  // namespace GridFormat::Traits
```


### Notes for Parallel Grids

In order to use `GridFormat` to write grid files from parallel computations using [MPI](https://de.wikipedia.org/wiki/Message_Passing_Interface),
make sure that you implement the above traits such that they __only__ return information on one __partition__. Moreover,
`GridFormat` currently does not provide any means to flag cells/points as ghost or overlap entities, and therefore, it is
expected that you only provide information on the collection of interior entities, that is, the partitions are expected
to be disjoint. To be more concrete, it is expected that

- The `Cells` trait provides a range over the interior cells of a partition, only.
- The `Points` trait provides a range over only those points that are connected to interior cells (otherwise there will be unconnected points in the output, however, it should still work).
- The `Extents` trait provides the number of cells of the partition, __not__ counting any ghosts or overlap cells.
- The `Ordinates` trait provides the ordinates of the partition, __only__ including points that are connected to interior cells.
- The `Origin` trait returns the lower-left corner of the partition __without__ ghost cells.

If you include ghost entities in your traits, output should still work, but there will be overlapping cells.

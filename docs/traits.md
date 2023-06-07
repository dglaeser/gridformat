<!-- SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

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
the traits specialization may look. For more details on this, please have a look at the resources referenced in the
[main readme](https://github.com/dglaeser/gridformat/blob/main).

<details>

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

</details>

As discussed in our
[overview over supported kinds of grids](https://github.com/dglaeser/gridformat/blob/main/docs/grid_kinds.md),
`GridFormat` understands the notion of unstructured, structured, rectilinear or image grids. The reason for this is that some
file formats are designed for specific kinds of grids and can store the information on their topology in a space-efficient manner.
To see which format assumes which kind of grid, see the [API documentation (coming soon)](https://github.com/dglaeser/gridformat).
In the code, there exist [concepts](https://en.cppreference.com/w/cpp/language/constraints) for each of these kinds of grids,
which essentially check if the required traits are correctly implemented. The remainder of this page discusses the different
traits used by `GridFormat`, and which ones need to be specialized in order to model a particular kind of grid. In case you want to use `GridFormat` in parallel computations, please also make sure to read the related section at the end of this page.
Note that all traits presented in the following are declared in the `namespace GridFormat::Traits`.


### Mandatory Traits

#### `template<typename Grid> struct Cells;`

This trait exposes how one can iterate over the cells of a grid. Specializations must provide a static function `get(const Grid&)`
that returns a [forward range](https://en.cppreference.com/w/cpp/ranges/forward_range) of cells.

#### `template<typename Grid> struct Points;`

This trait exposes how one can iterate over the points of a grid. Specializations must provide a static function `get(const Grid&)`
that returns a [forward range](https://en.cppreference.com/w/cpp/ranges/forward_range) of points.


### Traits for Unstructured Grids

#### `template<typename Grid, typename Cell> struct CellPoints;`

This trait exposes how one can iterate over the points of an individual grid cell. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns a [range](https://en.cppreference.com/w/cpp/ranges/range) of points that
make up the boundary of the given cell.

#### `template<typename Grid, typename Cell> struct CellType;`

This trait exposes the geometry type of an individual grid cell. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns an instance of the
[CellType enum](https://github.com/dglaeser/gridformat/blob/main/gridformat/grid/cell_type.hpp).

#### `template<typename Grid, typename Point> struct PointCoordinates;`

This trait exposes the coordinates of a grid point. Specializations must provide a static function `get(const Grid&, const Point&)`
that returns the coordinates of the given point as a
[statically sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize) (see section on `StaticSize` below).

#### `template<typename Grid, typename Point> struct PointId;`

This trait exposes a unique id for individual points of a grid. Specializations must provide a static function
`get(const Grid&, const Point&)` that returns a unique id (as integer value, e.g. `std::size_t`) for the given point.


### Traits for Structured Grids

In addition to the traits below, the `StructuredGrid` concept also requires that the `PointCoordinates` trait is implemented
(see above).

#### `template<typename Grid> struct Extents;`

This trait exposes the number of cells of the structured grid in each coordinate direction. Specializations must provide
a static function `get(const Grid&)` that returns a
[statically sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize), whose size is equal to the dimension of the grid.

#### `template<typename Grid, typename Entity> struct Location;`

This trait exposes the index tuple of a given entity within the structured grid. For a visualization and the assumptions on the
orientation, see the
[overview over supported kinds of grids](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/grid_kinds.md).
This trait must be specialized for both cells and points (i.e. for two types of `Entity`), and they must provide a static function
`get(const Grid&, const Entity&)` that returns a
[statically sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize)
whose elements are integer values (the indices of the entity) and whose size is equal to the dimension of the grid.


### Traits for Rectilinear Grids

In addition to the `Ordinates` trait below, the `RectilinearGrid` concept also requires that the `Extents` and `Location` traits
are implemented (see above).

#### `template<typename Grid> struct Ordinates;`

This trait exposes the ordinates of a `RectilinearGrid` grid along the coordinate axes.
Specializations must provide a static function `get(const Grid&, unsigned int direction)` that returns a
[range](https://en.cppreference.com/w/cpp/ranges/range) over the ordinates along the axis specified by `direction` (`direction < dim`, where dim is the dimension of the grid). The size of the range must be equal to the number of cells + 1 in the given
direction.


### Traits for Image Grids

In addition to the traits below, the `ImageGrid` concept also requires that the `Extents` and `Location` traits are
implemented (see above).

#### `template<typename Grid> struct Origin;`

This trait exposes the position of the lower-left corner of the grid, that
is, the position of the point at index $(0, 0, 0)$ (for a visualization see
[here](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/grid_kinds.md)).
Specializations must provide a static function `get(const Grid&)` that returns a
[statically sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize),
whose size is equal to the dimension of the grid.

#### `template<typename Grid> struct Spacing;`

This trait exposes the spacing between grid points along the axes (in other words, the size of the cells in each coordinate
direction). Specializations must provide a static function `get(const Grid&)` that returns a
[statically sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize), whose size is equal to the dimension of the grid.


### Optional Traits

#### `template<typename Grid> struct NumberOfPoints;`

This trait exposes the number of points of a grid, and if not specialized, the number of points is deduced from the size of the
range obtained from `Points`. If your point range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&)` that returns the number of points as an integral value.

#### `template<typename Grid> struct NumberOfCells;`

This trait exposes the number of cells of a grid, and if not specialized, the number of cells is deduced from the size of the
range obtained from `Cells`. If your cell range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&)` that returns the number of cells as an integral value.

#### `template<typename Grid, typename Cell> struct NumberOfCellPoints;`

This trait exposes the number of points in a grid cell, and if not specialized, the number of points is deduced from the size of the
range obtained from `CellsPoints`. If that range is not a [sized range](https://en.cppreference.com/w/cpp/ranges/sized_range),
however, specializing this trait can lead to an improved efficiency. Specializations must provide a static function
`get(const Grid&, const Cell&)` that returns the number of points in the cell as an integral value.

#### `template<typename Grid> struct Basis;`

This trait can be specified for image grids in order to specify their orientation. Per default, an image grid is assumed to be
axis-aligned, that is, the default basis (in 3D) is

```cpp
const auto default_basis = std::array{
    std::array{1.0, 0.0, 0.0},
    std::array{0.0, 1.0, 0.0},
    std::array{0.0, 0.0, 1.0},
};
```

Specializing the `Basis` trait, you can implement the static function `get(const Grid&)` and return a custom basis. It is expected
that the return type from this function is a two-dimensional,
[statically-sized range](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/docs/traits.md#templatetypename-t-struct-staticsize),
with the outer and inner dimensions being equal to the grid dimension.


#### `template<typename T> struct StaticSize;`

`GridFormat` requires that the types returned from some traits model the [`StaticallySizedRange` concept](https://github.com/dglaeser/gridformat/blob/feature/high-level-docs/gridformat/common/concepts.hpp#L25).
Statically sized means that the size of the [range](https://en.cppreference.com/w/cpp/ranges/range) is known at compile time.
Per default, `GridFormat` accepts `std::array`, `std::span` (with non-dynamic extent), `T[N]` or anything that either has a
`constexpr static std::size_t size()` function or a member variable named `size` that can be evaluated at compile time.
If your type does not fulfill any of these requirements, but is in fact range with a size known at compile-time, you can also
specialize the [`StaticSize` trait](https://github.com/dglaeser/gridformat/blob/main/gridformat/common/type_traits.hpp#L349)
for the type you want to return. Alternatively, you can of course just convert your type into an `std::array` within the
trait that expects you to return a statically sized range.


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

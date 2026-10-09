<!--SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>-->
<!--SPDX-License-Identifier: MIT-->

# `GridFormat` 0.6.0

## Fixes

- Several free functions defined in headers were not marked `inline`, which caused linker errors (multiple definitions)
when including `GridFormat` headers in more than one translation unit of the same program. A test now includes all
headers in two translation units linked into one executable.

- Writers did not check if the output file could be opened or if writing to it succeeded, such that e.g. writing into
a non-existing directory or onto a full disk silently produced no output. All file writers now throw an `IOError` in
these cases.

# `GridFormat` 0.5.0

## Fixes

- Some compiler versions raised errors because of the way `Field` instances exported data to `vector<bool>` using `std::ranges`.
Newer compilers seem to be ok with the code, but nevertheless, it was rewritten to work also with compilers that didn't swallow the old code.

- __VTKHDF__: the readers can now open files written by `VTK`. They failed on the `Type` attribute, which `VTK` writes as a
scalar, and rejected all files of a newer minor version of the format, although the specification guarantees that
minor versions are compatible. Only the major version is checked now.

- __VTKHDF__: transient files now store field data in the layout written by `VTK`: the tuples of all steps are
concatenated, and `FieldDataOffsets` and `FieldDataSizes` state where those of each step are and how many components
and tuples they have. Previously, numeric field data was stored as one row per step, which `VTK` read as a single
tuple with as many components as values (in image data files, it was not readable at all), and the number of values
could not change between steps. Files written with the previous layout can still be read, and so can transient files
written by `VTK`, which `GridFormat` previously misread. Field data with more than two dimensions is now written as
tuples with the remaining dimensions flattened into components, e.g. `(2, 3, 4)` as 2 tuples with 12 components, in
transient and non-transient files. `VTK` cannot read field data with more than two dimensions, and this is how tensors
in point and cell data are written as well. Reading such a file back yields the flattened shape.

- __VTKHDF__: the image data reader ignored `FieldDataOffsets` and read the meta data of a time step at the index of
the step, which failed for files with static meta data, where it is only written once.

- __VTKHDF__: `gridformat/vtk/hdf_reader.hpp` can now be included on its own; it previously relied on
`gridformat/vtk/common.hpp` being included before.

## Features

- __VTKHDF__: string meta data is now written as a dataset of variable-length strings instead of an array of ascii
codes with a trailing null terminator. `VTK`/`ParaView` read such datasets as `vtkStringArray` and show the text in
the information panel, whereas the previous layout showed up as a numeric char array. This brings the `VTKHDF` writers
in line with the `VTK-XML` ones, which have always written string meta data as `String` data arrays. Note that reading
string meta data from `VTKHDF` files requires `VTK` 9.4 or newer; use one of the `VTK-XML` formats if you need to
support older readers.

- __Meta data__: string meta data may now hold multiple strings, in all `VTK` formats. Fields hold them as a single
sequence of characters in which each string is terminated by `'\0'`, as in the `VTK-XML` formats, and the new
`MultiString` type helps to create and split them: `writer.set_meta_data("names", MultiString{{"a", "b"}})`, and
`reader.meta_data_field("names")->export_to<MultiString>().slices()`. Exporting a field into a `std::string` now strips
the trailing `'\0'` and raises an error if the field holds more than one string. Previously, reading a string from a
`VTK-XML` file returned it including the terminator.

- __VTKHDF__: as a consequence of the above, meta data fields whose value type is `char` now end up as strings rather
than as arrays of ascii codes, with every `'\0'` separating two strings. This is how the `VTK-XML` formats have always
treated them, but it is a change for `VTKHDF`. Give `char` fields that hold numbers instead of text an explicit integer
precision to keep them numeric.

- __VTKHDF__: the writers now reject field names containing `/` or `.`, which cannot be represented as `VTKHDF` array
names (the `VTK-XML` formats do not have this restriction).

- __VTKHDF__: in transient files, writing a field with a different precision than in a previous step now raises an
error instead of being converted silently by `hdf5`, which could truncate values (e.g. `double` to `int`).

- __Traits__: the predefined traits for `dolfinx` are now written for and tested with `dolfinx` 0.11, and support for
`dolfinx` 0.6 has been dropped. Since meshes and function spaces in `dolfinx` are templates over the geometry type,
`GridFormat::DolfinX::LagrangePolynomialGrid` is now a class template `LagrangePolynomialGrid<T>`. It can be constructed
from a function space without specifying `T`, or created via the new `GridFormat::DolfinX::make_lagrange_grid(space)`.

- __Traits__: the predefined traits are now tested with `dune` 2.11, `deal.II` 9.7.1, `CGAL` 6.2.1 and `mfem` 4.10.
The `dune` traits are now also tested with `dune-alugrid` grids when compiling with `clang`.

- The regression tests now run against a newer version of `vtk`, namely `v9.7.1`, which is now pinned in the docker
image. Generating the `VTKHDF` test data requires `vtk` 9.7 or newer.

- The test suite has a new `cmake` option `GRIDFORMAT_GENERATE_VTK_TEST_FILES` (default: `ON`), which can be used
to skip the generation of test files with `VTK`. The tests now run with `gcc-14` and `clang-22` on `ubuntu 26.04`.

# `GridFormat` 0.4.0

## Features

- __Traits__: cell ranges as defined via the `Cells` trait are no longer required to be const-iterable.

- __Writers__: writers can now take fields whose value type is `bool`, and fields can now also be exported into ranges with `bool` as range value type.

- __Decorators__: added a decorator for readers that exposes polyline cells as subdivided into its individual segments.

## Removed interfaces

- __Common__:
    - The `FilteredRange` wrapper has been removed since the cell ranges are no longer required to be const-iterable.

# `GridFormat` 0.3.0

## Features

- __Writers__: the custom range adaptors & iterators used by the writers under the hood have been changed to support range sentinels of different type than the range iterator. This makes it easier to specialize the traits for grid implementations that use sentinels of different type than the grid entity iterators.
- __Traits__: support for writing `Dune::FieldMatrix` as tensor field data.
- __Reader__: the `Reader` class now allows for opening a file upon instantiation using `GridFormat::Reader::from(filename)` (taking further optional constructor arguments). Moreover, you can now open a file and receive the modified reader as return value using `reader.with_opened(filename)`.
- __VTK__: VTK-XML files of the older file format version 0.1 can now also be read by all vtk readers

## Deprecated interfaces

- __Common__:
    - the `get_md_layout<SubRange>(std::size_t)` overload is deprecated as it may interfere with `get_md_layout<Range>(Range)` if `Range` is constructible from an `std::size_t` and the template argument `Range` is explicitly specified. The intented behaviour can now be achieved with `MDLayout{{std::size_t}}.with_sub_layout_from<SubRange>()`.
    - related to the above, an `MDLayout` for a scalar value now has a dimension of zero to distinguish it from a vector of size 1.

## Continuous integration

- on PRs, a bunch of performance benchmarks are run with the code of the PR and the target branch, and observed runtime differences are posted as a comment to the pull request.
- the CI helper scripts, for instance, to select tests affected by changes, have been moved from the `/bin` to the  `/test` directory.
- the runtime of the test pipeline has been reduced by avoiding duplicate test runs and skipping those tests in pull requests for which no changes are introduced.
- the CI now runs on ubuntu:24.04 and tests the code with `gcc-14` and `clang-18`.

# `GridFormat` 0.2.0

## Features

- __Documentation__: the available file formats are now listed as a separate Doxygen group.
- __CI__: for PRs, only the tests affected by changes in the PR are built and run in order to speed up the workflows.
- __Field__: `Field::export` now accepts r-values, which will be used to populate the values and return them again. Thus, one can now write `const auto values = field.export_to(std::vector<double>{})`, where `auto` will be `std::vector<double>`.
- __Traits__: added a `GridFactoryAdapter` for `Dune::GridFactory` to the dune traits to facilitate exporting read grids into dune grids.

# `GridFormat` 0.1.2

## Fixes

- Fixed description extraction from changelog for releases.

# `GridFormat` 0.1.1

## Fixes

- Fixed the artifact path in the release workflow.

# `GridFormat` 0.1.0

Our very first release 🎉

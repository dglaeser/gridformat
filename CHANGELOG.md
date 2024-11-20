<!--SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>-->
<!--SPDX-License-Identifier: MIT-->

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

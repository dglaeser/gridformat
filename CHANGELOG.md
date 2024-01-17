<!--SPDX-FileCopyrightText: 2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>-->
<!--SPDX-License-Identifier: MIT-->

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

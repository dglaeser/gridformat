<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Write out discretized analytical expressions

This example illustrates how to use `GridFormat` to write out an analytical expression into a grid
file for subsequent visualization with e.g. [ParaView](https://www.paraview.org/). This task does
not require the specialization of any traits, as one can use the predefined `ImageGrid` class to
define the discretization on which to write out.

What you can see in this example:

- how to create a `GridFormat::Writer` and attach point/cell data to it.
- how to read point/cell values back in from a grid file with the `GridFormat::Reader`.

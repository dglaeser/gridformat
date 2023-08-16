<!-- SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de> -->
<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Reading and writing files in parallel

This example illustrates how to read and write data from and to grid files in parallel.
Essentially, what one has to do is to pass a communicator to the `Reader` and `Writer`
classes such that they can communicate the required data. When passing a communicator
to the reader, it will read parallel file formats such that each processor only reads
its corresponding piece of the data. When constructing the reader without a communicator
(or when explicitly passing `GridFormat::null_communicator`), it will read the entire file
and merge all pieces.

In the context of simulations, grid data structures typically require additional information
on the processor boundaries, ghost cells, etc. Since this is usually strongly dependent on
the use case and the data structured involved, this has to be done on the user side.
`GridFormat` only reads in the pieces as they are defined in the parallel grid files.


What you can see in this example is:

- how to write a parallel (`.pvtu`) file
- how to read in a parallel file, where each process only reads a piece of the data
- how to read in all pieces of a parallel file on one process

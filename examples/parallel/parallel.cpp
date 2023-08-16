// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <sstream>

#include <mpi.h>

#include <gridformat/gridformat.hpp>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto comm = MPI_COMM_WORLD;
    const auto num_ranks = GridFormat::Parallel::size(comm);
    const auto rank = GridFormat::Parallel::rank(comm);
    if (num_ranks == 1)
        throw std::runtime_error("This example should be run in parallel. Retry with 'mpirun -n 2 parallel.");
    if (num_ranks%2 != 0)
        throw std::runtime_error("This example requires to be run with a number of ranks divisible by 2.");

    // let's create a grid such that the partitions touch
    const double x_offset = rank%2;
    const double y_offset = rank/2;
    GridFormat::ImageGrid<2, double> grid{
        {x_offset, y_offset},
        {1.0, 1.0},
        {10, 10}
    };

    // let's write a .pvtu file, where each process writes an individual piece
    GridFormat::Writer writer{GridFormat::vtu, grid, comm};
    writer.set_point_field("id", [&] (const auto& p) { return GridFormat::id(grid, p); });
    const auto filename = writer.write("point_ids");
    if (rank == 0)
        std::cout << "Wrote parallel vtu file into '" << filename << "'" << std::endl;

    // let's now read the file in parallel (i.e. each rank reads only its corresponding piece)
    GridFormat::Reader reader{GridFormat::vtu, comm};
    reader.open(filename);

    std::stringstream out;  // going via stringstream reduces the risk of intermingled output from the processors
    out << "Reader on rank " << rank << " has " << reader.number_of_points() << " points" << std::endl;
    out << "Reader on rank " << rank << " has " << reader.number_of_cells() << " cells" << std::endl;
    std::cout << out.str();

    // Alternatively, we can read in the parallel grid sequentially by concatenating all pieces.
    // Simply omitting the communicator in the constructur signals to use sequential I/O for parallel formats.
    GridFormat::Reader sequential_reader{GridFormat::vtu};
    sequential_reader.open(filename);

    std::stringstream seq_out;
    seq_out << "Sequential reader on rank " << rank << " has " << sequential_reader.number_of_points() << " points" << std::endl;
    seq_out << "Sequential reader on rank " << rank << " has " << sequential_reader.number_of_cells() << " cells" << std::endl;
    std::cout << seq_out.str();

    MPI_Finalize();
    return 0;
}

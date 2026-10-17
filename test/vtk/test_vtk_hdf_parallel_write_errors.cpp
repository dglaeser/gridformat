// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.a.glaeser@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>

#include <mpi.h>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../testing.hpp"

// create a directory at the path of the file to be written, such that opening it fails
void block_file(const std::string& filename) {
    std::filesystem::remove_all(filename);
    std::filesystem::create_directory(filename);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;

    const auto comm = MPI_COMM_WORLD;
    const int rank = GridFormat::Parallel::rank(comm);
    const GridFormat::Test::StructuredGrid<2> grid{
        {{1.0, 1.0}},
        {{2, 2}},
        {{static_cast<double>(rank), 0.0}}
    };

    "vtk_hdf_time_series_writer_throws_on_all_ranks_if_clearing_fails"_test = [&] () {
        // the file is cleared on the root rank only
        if (rank == 0)
            block_file("test_vtk_hdf_parallel_write_errors.hdf");
        GridFormat::VTKHDFTimeSeriesWriter writer{grid, comm, "test_vtk_hdf_parallel_write_errors"};
        const auto write = [&] () { writer.write(0.0); };
        if (rank == 0)
            expect(throws(write));  // HighFive exception from opening the file
        else
            expect(throws<GridFormat::IOError>(write));
    };

    MPI_Finalize();
    return 0;
}

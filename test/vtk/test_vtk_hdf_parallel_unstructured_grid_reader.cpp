// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>

#include <mpi.h>

#include <gridformat/vtk/hdf_unstructured_grid_writer.hpp>
#include <gridformat/vtk/hdf_unstructured_grid_reader.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const auto grid = GridFormat::Test::make_unstructured_2d(rank);
    const bool verbose = (rank == 0);

    {
        GridFormat::VTKHDFUnstructuredGridWriter writer{grid, MPI_COMM_WORLD};

        {
            GridFormat::VTKHDFUnstructuredGridReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_parallel_unstructured_test_file_2d_in_2d", {}, verbose);
        }
        {  // test also convenience reader
            GridFormat::VTKHDFReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_parallel_unstructured_test_file_2d_in_2d_from_generic", {}, verbose);
        }
    }
    {
        {
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid, MPI_COMM_WORLD, "reader_vtk_hdf_parallel_unstructured_time_series_2d_in_2d"
            };
            GridFormat::VTKHDFUnstructuredGridReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, [&] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, MPI_COMM_WORLD, filename};
            }, {}, verbose);
        }
        {  // test also convenience reader
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid, MPI_COMM_WORLD, "reader_vtk_hdf_parallel_unstructured_time_series_2d_in_2d_from_generic"
            };
            GridFormat::VTKHDFReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, [&] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, MPI_COMM_WORLD, filename};
            }, {}, verbose);
        }
    }

    MPI_Finalize();
    return 0;
}

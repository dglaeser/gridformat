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
#include "../testing.hpp"


template<typename Reader, typename Grid>
void test_sequentially_opened(const Reader& reader,
                              const Grid& piece_grid,
                              const std::size_t num_pieces,
                              double time_step = 1.0) {
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    const auto read_grid = [&] () {
        GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
        reader.export_grid(factory);
        return std::move(factory).grid();
    } ();
    const std::size_t num_expected_cells = GridFormat::number_of_cells(piece_grid)*num_pieces;
    const std::size_t num_expected_points = GridFormat::number_of_points(piece_grid)*num_pieces;
    std::size_t cell_count = 0;
    reader.visit_cells([&] (const auto&, const auto&) { cell_count++; });

    expect(eq(cell_count, num_expected_cells));
    expect(eq(reader.number_of_cells(), num_expected_cells));
    expect(eq(reader.number_of_points(), num_expected_points));
    expect(eq(GridFormat::number_of_cells(read_grid), num_expected_cells));
    expect(eq(GridFormat::number_of_points(read_grid), num_expected_points));

    for (const auto& [name, field_ptr] : cell_fields(reader)) {
        expect(field_ptr->layout().dimension() > 0);
        expect(eq(field_ptr->layout().extent(0), num_expected_cells));
        expect(GridFormat::Test::test_field_values<2>(
            name, field_ptr, read_grid, GridFormat::cells(read_grid), time_step
        ));
    }

    for (const auto& [name, field_ptr] : point_fields(reader)) {
        expect(field_ptr->layout().dimension() > 0);
        expect(eq(field_ptr->layout().extent(0), num_expected_points));
        expect(GridFormat::Test::test_field_values<2>(
            name, field_ptr, read_grid, GridFormat::points(read_grid), time_step
        ));
    }
}


template<typename Grid>
void test_sequential_io(const Grid& piece_grid,
                        const std::size_t num_pieces,
                        const std::string& filename,
                        double time_step = 1.0) {
    GridFormat::VTKHDFReader reader;
    reader.open(filename);
    test_sequentially_opened(reader, piece_grid, num_pieces, time_step);
}

template<typename Grid>
void test_sequential_time_series_io(const Grid& piece_grid,
                                 const std::size_t num_pieces,
                                 const std::string& filename) {
    GridFormat::VTKHDFReader reader;
    reader.open(filename);
    for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
        reader.set_step(step);
        test_sequentially_opened(reader, piece_grid, num_pieces, reader.time_at_step(step));
    }
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto num_ranks = GridFormat::Parallel::size(MPI_COMM_WORLD);
    const auto rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const auto grid = GridFormat::Test::make_unstructured_2d(rank);
    const bool verbose = (rank == 0);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    {
        GridFormat::VTKHDFUnstructuredGridWriter writer{grid, MPI_COMM_WORLD};

        const auto parallel_file = [&] () {
            GridFormat::VTKHDFUnstructuredGridReader reader{MPI_COMM_WORLD};
            const auto fn = test_reader<2, 2>(writer, reader, "reader_vtk_hdf_parallel_unstructured_test_file_2d_in_2d", {}, verbose);
            "parallel_vtk_hdf_unstructured_grid_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(num_ranks)));
            };
            return fn;
        } ();
        {  // test also convenience reader
            GridFormat::VTKHDFReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_parallel_unstructured_test_file_2d_in_2d_from_generic", {}, verbose);
            "parallel_vtk_hdf_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(num_ranks)));
            };
        }
        if (rank == 0)  // test also sequential I/O of parallel file
            "parallel_vtk_hdf_reader_sequential_io"_test = [&] () {
                std::cout << "Testing sequential I/O with " << parallel_file << std::endl;
                test_sequential_io(grid, num_ranks, parallel_file);
            };
        GridFormat::Parallel::barrier(MPI_COMM_WORLD);
    }
    {
        const auto parallel_file = [&] () {
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid, MPI_COMM_WORLD, "reader_vtk_hdf_parallel_unstructured_time_series_2d_in_2d"
            };
            GridFormat::VTKHDFUnstructuredGridReader reader{MPI_COMM_WORLD};
            const auto fn = test_reader<2, 2>(writer, reader, [&] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, MPI_COMM_WORLD, filename};
            }, {}, verbose);
            "parallel_vtk_hdf_unstructured_grid_time_series_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(num_ranks)));
            };
            return fn;
        } ();
        {  // test also convenience reader
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid, MPI_COMM_WORLD, "reader_vtk_hdf_parallel_unstructured_time_series_2d_in_2d_from_generic"
            };
            GridFormat::VTKHDFReader reader{MPI_COMM_WORLD};
            test_reader<2, 2>(writer, reader, [&] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, MPI_COMM_WORLD, filename};
            }, {}, verbose);
            "parallel_vtk_hdf_time_series_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(num_ranks)));
            };
        }
        if (rank == 0)  // test also sequential I/O of parallel file
            "parallel_vtk_hdf_reader_sequential_time_series_io"_test = [&] () {
                std::cout << "Testing sequential time series I/O with " << parallel_file << std::endl;
                test_sequential_time_series_io(grid, num_ranks, parallel_file);
            };
        GridFormat::Parallel::barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

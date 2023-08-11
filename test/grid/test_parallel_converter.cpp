// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/converter.hpp>

#include <gridformat/vtk/vtu_reader.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvd_reader.hpp>

#include "unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto comm = MPI_COMM_WORLD;
    const auto num_ranks = GridFormat::Parallel::size(comm);
    const auto rank = GridFormat::Parallel::rank(comm);
    if (num_ranks%2 != 0)
        throw std::runtime_error("This test requires that the number of ranks be divisible by 2");

    const double x_offset = static_cast<double>(rank%2);
    const double y_offset = static_cast<double>(rank/2);
    const GridFormat::Test::StructuredGrid<2> struct_grid{{1.0, 1.0}, {4, 5}, {x_offset, y_offset}};
    const auto test_data = GridFormat::Test::make_test_data<2>(struct_grid, GridFormat::float64);

    // let each rank write a .vtu file
    GridFormat::VTUWriter piece_writer{struct_grid};
    GridFormat::Test::add_test_data(piece_writer, test_data, GridFormat::float32);
    const auto piece_filename = piece_writer.write("parallel_converter_vtu_2d_in_2d_in-" + std::to_string(rank));
    std::cout << "Wrote '" << GridFormat::as_highlight(piece_filename) << "'" << std::endl;

    GridFormat::VTUReader reader;
    reader.open(piece_filename);

    std::cout << "Converting to .pvtu" << std::endl;
    GridFormat::convert(reader, "parallel_converter_pvtu_2d_in_2d_out", [&] (const auto& grid) {
        return GridFormat::PVTUWriter{grid, comm};
    });

    // test a time series
    GridFormat::PVDWriter time_series_writer{
        GridFormat::PVTIWriter{struct_grid, comm},
        "parallel_time_series_pvti_2d_in_2d_in"
    };
    const auto test_ts_filename = [&] () {
        std::string ts_filename;
        for (std::size_t i = 0; i < 5; ++i) {
            const auto time_step = static_cast<double>(i)*0.2;
            const auto test_data = GridFormat::Test::make_test_data<2>(struct_grid, GridFormat::float64, time_step);
            GridFormat::Test::add_test_data(time_series_writer, test_data, GridFormat::float32);
            ts_filename = time_series_writer.write(time_step);
        }
        return ts_filename;
    } ();

    std::cout << "Converting parallel pvd/vti to parallel pvd/vtu time series" << std::endl;
    GridFormat::PVDReader time_series_reader{comm};
    time_series_reader.open(test_ts_filename);
    GridFormat::convert(time_series_reader, [&] (const auto& grid) {
        return GridFormat::PVDWriter{
            GridFormat::PVTUWriter{grid, comm},
            "parallel_converter_time_series_pvtu_2d_in_2d"
        };
    });

    MPI_Finalize();
    return 0;
}

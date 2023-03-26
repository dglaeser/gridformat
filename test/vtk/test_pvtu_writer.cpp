// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    auto grid = GridFormat::Test::make_unstructured_2d<2>();
    auto writer = GridFormat::PVTUWriter{grid, MPI_COMM_WORLD}
        .with_encoding(GridFormat::Encoding::base64)
        .with_compression(GridFormat::none)
        .with_header_precision(GridFormat::uint32)
        .with_coordinate_precision(GridFormat::float32);

    const auto data = GridFormat::Test::make_test_data<2, double>(grid);
    GridFormat::Test::add_test_data(writer, data, GridFormat::Precision<float>{});

    const auto filename = writer.write("pvtu_2d_in_2d");
    if (GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0)
        std::cout << "Wrote " << GridFormat::as_highlight(filename) << std::endl;

    MPI_Finalize();
    return 0;
}

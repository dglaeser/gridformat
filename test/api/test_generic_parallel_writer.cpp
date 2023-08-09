// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <mpi.h>

#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"

template<typename Writer, typename Comm>
void write(Writer&& writer, const Comm& comm, std::string suffix = "", std::string prefix = "") {
    const auto& grid = writer.grid();
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("point_func", [&] (const auto& p) {
        return GridFormat::Test::test_function<double>(grid.position(p));
    });
    writer.set_cell_field("cell_func", [&] (const auto& c) {
        return GridFormat::Test::test_function<double>(grid.center(c));
    });

    suffix = suffix != "" ? "_" + suffix : "";
    prefix = prefix != "" ? prefix + "_" : "";
    const auto filename = writer.write(prefix + "generic_parallel_2d_in_2d" + suffix);

    if (GridFormat::Parallel::rank(comm) == 0)
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    if (GridFormat::Parallel::size(MPI_COMM_WORLD)%2 != 0)
        throw GridFormat::ValueError("Communicator size must be a multiple of 2");

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    GridFormat::ImageGrid<2, double> grid{
        {xoffset, yoffset},
        {1.0, 1.0},
        {10, 15}
    };

    write(
        GridFormat::Writer{GridFormat::vtu({.encoder = GridFormat::Encoding::ascii}), grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{GridFormat::vti({.encoder = GridFormat::Encoding::raw}), grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{GridFormat::vtr({.data_format = GridFormat::VTK::DataFormat::appended}), grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{GridFormat::vts({.compressor = GridFormat::none}), grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{GridFormat::vtp({}), grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{GridFormat::any, grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD,
        "from_any"
    );
#if GRIDFORMAT_HAVE_HIGH_FIVE
    write(
        GridFormat::Writer{GridFormat::vtk_hdf, grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD,
        "unstructured"
    );
    write(
        GridFormat::Writer{GridFormat::FileFormat::VTKHDFUnstructured{}, grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD,
        "unstructured_explicit"
    );
    write(
        GridFormat::Writer{GridFormat::FileFormat::VTKHDFImage{}, grid, MPI_COMM_WORLD},
        MPI_COMM_WORLD,
        "image_explicit",
        "_ignore_regression"
    );
#endif

    MPI_Finalize();
    return 0;
}

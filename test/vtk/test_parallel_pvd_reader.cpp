// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvd_reader.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"

template<typename PieceReader,
         template<typename...> typename PieceWriter,
         typename Communicator>
void test_pvd(const std::string& acronym, const Communicator& comm) {
    const auto size = GridFormat::Parallel::size(comm);
    const auto rank = GridFormat::Parallel::rank(comm);
    if (size%2 != 0)
        throw GridFormat::ValueError("Communicator size has to be a multiple of 2 for this test");

    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);

    const GridFormat::Test::StructuredGrid<2> grid{
        {1.0, 1.0},
        {4, 5},
        {xoffset, yoffset}
    };
    GridFormat::PVDWriter writer{
        PieceWriter{grid, comm},
        "reader_pvd_parallel_with_" + acronym + "_2d_in_2d",
    };
    GridFormat::PVDReader reader{comm};
    GridFormat::Test::test_reader<2, 2>(writer, reader, [&] (const auto& grid, const auto& filename) {
        return GridFormat::PVDWriter{GridFormat::PVTUWriter{grid, comm}, filename};
    });
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    test_pvd<GridFormat::PVTUReader, GridFormat::PVTUWriter>("pvtu", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTPReader, GridFormat::PVTPWriter>("pvtp", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTIReader, GridFormat::PVTIWriter>("pvti", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTRReader, GridFormat::PVTRWriter>("pvtr", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTSReader, GridFormat::PVTSWriter>("pvts", MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}

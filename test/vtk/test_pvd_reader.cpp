// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vts_writer.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvd_reader.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"

template<typename PieceReader, template<typename...> typename PieceWriter>
void test_pvd(const std::string& acronym) {
    const GridFormat::Test::StructuredGrid<2> grid{
        {1.0, 1.0},
        {4, 5}
    };
    GridFormat::PVDWriter writer{
        PieceWriter{grid},
        "reader_pvd_sequential_with_" + acronym + "_2d_in_2d"
    };
    GridFormat::PVDReader reader;
    GridFormat::Test::test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
        return GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, filename};
    });
}

int main() {
    test_pvd<GridFormat::VTUReader, GridFormat::VTUWriter>("vtu");
    test_pvd<GridFormat::VTPReader, GridFormat::VTPWriter>("vtp");
    test_pvd<GridFormat::VTIReader, GridFormat::VTIWriter>("vti");
    test_pvd<GridFormat::VTRReader, GridFormat::VTRWriter>("vtr");
    test_pvd<GridFormat::VTSReader, GridFormat::VTSWriter>("vts");
    return 0;
}

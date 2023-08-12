// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vts_writer.hpp>

#include <memory>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvd_reader.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"


template<typename PieceReader, template<typename...> typename PieceWriter>
std::string test_pvd(const std::string& acronym) {
    const GridFormat::Test::StructuredGrid<2> grid{
        {1.0, 1.0},
        {4, 5}
    };
    GridFormat::PVDWriter writer{
        PieceWriter{grid},
        "reader_pvd_sequential_with_" + acronym + "_2d_in_2d"
    };
    GridFormat::PVDReader reader;
    return GridFormat::Test::test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
        return GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, filename};
    });
}

int main() {
    const auto pvd_vtu_file = test_pvd<GridFormat::VTUReader, GridFormat::VTUWriter>("vtu");
    const auto pvd_vtp_file = test_pvd<GridFormat::VTPReader, GridFormat::VTPWriter>("vtp");
    test_pvd<GridFormat::VTIReader, GridFormat::VTIWriter>("vti");
    test_pvd<GridFormat::VTRReader, GridFormat::VTRWriter>("vtr");
    test_pvd<GridFormat::VTSReader, GridFormat::VTSWriter>("vts");

    // test if pvd reader bound to a specific piece reader fails upon read
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    GridFormat::PVDReader pvd_vtu_reader{[] (const std::string&) {
        return std::make_unique<GridFormat::VTUReader>();
    }};

    "pvd_reader_name"_test = [&] () {
        expect(pvd_vtu_reader.name() == "PVDReader");
    };

    "bound_pvd_reader_throws_with_wrong_piece_format"_test = [&] () {
        expect(throws([&] () { pvd_vtu_reader.open(pvd_vtp_file); }));
    };

    "bound_pvd_reader_reads_matching_piece_format"_test = [&] () {
        pvd_vtu_reader.open(pvd_vtu_file);
        expect(eq(pvd_vtu_reader.number_of_cells(), std::size_t{20}));
        expect(eq(pvd_vtu_reader.number_of_points(), std::size_t{30}));
    };

    "pvd_reader_number_of_pieces"_test = [&] () {
        expect(eq(pvd_vtu_reader.number_of_pieces(), std::size_t{1}));
    };

    return 0;
}

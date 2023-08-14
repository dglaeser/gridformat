// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <string>
#include <algorithm>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>
#include <gridformat/vtk/pvti_writer.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>
#include <gridformat/vtk/pvts_writer.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvd_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

template<typename PieceReader,
         template<typename...> typename PieceWriter,
         typename Communicator>
std::string test_pvd(const std::string& acronym, const Communicator& comm) {
    const auto size = GridFormat::Parallel::size(comm);
    const auto rank = GridFormat::Parallel::rank(comm);
    if (size%2 != 0)
        throw GridFormat::ValueError("Communicator size has to be a multiple of 2 for this test");

    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    const std::size_t nx = 4;
    const std::size_t ny = 5;
    const GridFormat::Test::StructuredGrid<2> grid{{1.0, 1.0}, {nx, ny}, {xoffset, yoffset}};

    GridFormat::PVDWriter writer{
        PieceWriter{grid, comm},
        "reader_pvd_parallel_with_" + acronym + "_2d_in_2d",
    };
    GridFormat::PVDReader reader{comm};
    const auto test_filename = GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        [&] (const auto& grid, const auto& filename) {
            return GridFormat::PVDWriter{GridFormat::PVTUWriter{grid, comm}, filename};
    });

    std::vector<std::string> pfield_names;
    std::vector<std::string> cfield_names;
    std::vector<std::string> mfield_names;
    std::ranges::copy(point_field_names(reader), std::back_inserter(pfield_names));
    std::ranges::copy(cell_field_names(reader), std::back_inserter(cfield_names));
    std::ranges::copy(meta_data_field_names(reader), std::back_inserter(mfield_names));

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "pvd_reader_parallel_number_of_pieces"_test = [&] () {
        expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(size)));
    };

    if (rank == 0) {
        "pvd_reader_parallel_file_as_sequential"_test = [&] () {
            std::cout << "Testing to read rewritten test file '" << test_filename << "' sequentially" << std::endl;

            GridFormat::PVDReader sequential_reader;
            sequential_reader.open(test_filename);

            const auto sequential_grid = [&] () {
                GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
                sequential_reader.export_grid(factory);
                return std::move(factory).grid();
            } ();

            expect(eq(GridFormat::number_of_cells(sequential_grid), GridFormat::number_of_cells(grid)*size));
            expect(eq(sequential_reader.number_of_cells(), GridFormat::number_of_cells(grid)*size));

            for (unsigned int step = 0; step < sequential_reader.number_of_steps(); ++step) {
                sequential_reader.set_step(step);
                const auto t = sequential_reader.time_at_step(step);

                expect(std::ranges::equal(point_field_names(sequential_reader), pfield_names));
                expect(std::ranges::equal(cell_field_names(sequential_reader), cfield_names));
                expect(std::ranges::equal(meta_data_field_names(sequential_reader), mfield_names));

                for (const auto& [name, field_ptr] : point_fields(sequential_reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, sequential_grid, GridFormat::points(sequential_grid), t
                    ));

                for (const auto& [name, field_ptr] : cell_fields(sequential_reader))
                    expect(GridFormat::Test::test_field_values<2>(
                        name, field_ptr, sequential_grid, GridFormat::cells(sequential_grid), t
                    ));
            }
        };
    }

    GridFormat::Parallel::barrier(comm);
    return test_filename;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto pvd_vtu_file = test_pvd<GridFormat::PVTUReader, GridFormat::PVTUWriter>("pvtu", MPI_COMM_WORLD);
    const auto pvd_vtp_file = test_pvd<GridFormat::PVTPReader, GridFormat::PVTPWriter>("pvtp", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTIReader, GridFormat::PVTIWriter>("pvti", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTRReader, GridFormat::PVTRWriter>("pvtr", MPI_COMM_WORLD);
    test_pvd<GridFormat::PVTSReader, GridFormat::PVTSWriter>("pvts", MPI_COMM_WORLD);

    // test if pvd reader bound to a specific piece reader fails upon read
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    GridFormat::PVDReader pvd_vtu_reader{MPI_COMM_WORLD, [] (const auto& communicator, const std::string&) {
        return std::make_unique<GridFormat::PVTUReader>(communicator);
    }};

    "bound_parallel_pvd_reader_throws_with_wrong_piece_format"_test = [&] () {
        expect(throws([&] () { pvd_vtu_reader.open(pvd_vtp_file); }));
    };

    "bound_parallel_pvd_reader_reads_matching_piece_format"_test = [&] () {
        pvd_vtu_reader.open(pvd_vtu_file);
        expect(eq(pvd_vtu_reader.number_of_cells(), std::size_t{20}));
        expect(eq(pvd_vtu_reader.number_of_points(), std::size_t{30}));
    };

    MPI_Finalize();
    return 0;
}

// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <vector>
#include <ranges>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvts_reader.hpp>
#include <gridformat/vtk/pvts_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto comm = MPI_COMM_WORLD;
    const auto rank = GridFormat::Parallel::rank(comm);
    const auto size = GridFormat::Parallel::size(comm);
    if (size%2 != 0)
        throw GridFormat::ValueError("This test requires that the number of ranks is a multiple of 2");

    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    const std::size_t nx = 4;
    const std::size_t ny = 5;

    const auto grid = GridFormat::Test::StructuredGrid<2>{{1.0, 1.0}, {nx, ny}, {xoffset, yoffset}};
    GridFormat::PVTSWriter writer{grid, comm};
    GridFormat::PVTSReader reader{comm};
    const auto test_filename = GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        "reader_pvts_test_file_2d_in_2d",
        {},
        (rank == 0)
    );

    std::vector<std::string> pfield_names;
    std::vector<std::string> cfield_names;
    std::vector<std::string> mfield_names;
    std::ranges::copy(point_field_names(reader), std::back_inserter(pfield_names));
    std::ranges::copy(cell_field_names(reader), std::back_inserter(cfield_names));
    std::ranges::copy(meta_data_field_names(reader), std::back_inserter(mfield_names));

    const std::size_t num_domains_x = 2;
    const std::size_t num_domains_y = size/2;
    const std::size_t num_total_cells = (num_domains_x*nx)*(num_domains_y*ny);
    const std::size_t num_total_points = (num_domains_x*nx + 1)*(num_domains_y*ny + 1);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "pvts_reader_name"_test = [&] () {
        expect(reader.name() == "PVTSReader");
    };

    "parallel_pvts_read_number_of_pieces"_test = [&] () {
        expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(size)));
    };

    // test that sequential I/O yields the expected results
    if (rank == 0) {
        std::cout << "Opening '" << GridFormat::as_highlight(test_filename) << "'" << std::endl;
        GridFormat::PVTSReader sequential_reader{};
        sequential_reader.open(test_filename);

        "sequential_pvts_read_number_of_entities"_test = [&] () {
            expect(eq(sequential_reader.number_of_cells(), num_total_cells));
            expect(eq(sequential_reader.number_of_points(), num_total_points));
        };

        "sequential_pvts_read_field_names"_test = [&] () {
            expect(std::ranges::equal(point_field_names(sequential_reader), pfield_names));
            expect(std::ranges::equal(cell_field_names(sequential_reader), cfield_names));
            expect(std::ranges::equal(meta_data_field_names(sequential_reader), mfield_names));
        };

        const auto sequential_grid = [&] () {
            GridFormat::Test::UnstructuredGridFactory<2, 3> factory;
            sequential_reader.export_grid(factory);
            return std::move(factory).grid();
        } ();

        // write the result as unstructured grid to regression-test it
        GridFormat::VTUWriter vtu_writer{sequential_grid};
        GridFormat::Test::add_meta_data(vtu_writer);

        "sequential_pvts_read_point_fields"_test = [&] () {
            for (const auto& [name, field_ptr] : point_fields(sequential_reader)) {
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::points(sequential_grid)
                ));
                vtu_writer.set_point_field(name, field_ptr);
            }
        };

        "sequential_pvts_read_cell_fields"_test = [&] () {
            for (const auto& [name, field_ptr] : cell_fields(sequential_reader)) {
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::cells(sequential_grid)
                ));
                vtu_writer.set_cell_field(name, field_ptr);
            }
        };

        const auto seq_filename = vtu_writer.write(
            "reader_pvts_test_file_2d_in_2d_rewritten_as_vtu_rank_" + std::to_string(rank)
        );
        std::cout << "Wrote '" << GridFormat::as_highlight(seq_filename) << "'" << std::endl;
    }

    MPI_Finalize();
    return 0;
}

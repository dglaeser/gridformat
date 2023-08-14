// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <vector>
#include <ranges>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvtp_reader.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto comm = MPI_COMM_WORLD;
    const auto rank = GridFormat::Parallel::rank(comm);
    const auto grid = GridFormat::Test::make_unstructured_2d(rank);
    GridFormat::PVTPWriter writer{grid, comm};
    GridFormat::PVTPReader reader{comm};
    const auto test_filename = GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        "reader_pvtp_test_file_2d_in_2d",
        {},
        (rank == 0)
    );

    std::vector<std::string> pfield_names;
    std::vector<std::string> cfield_names;
    std::vector<std::string> mfield_names;
    std::ranges::copy(point_field_names(reader), std::back_inserter(pfield_names));
    std::ranges::copy(cell_field_names(reader), std::back_inserter(cfield_names));
    std::ranges::copy(meta_data_field_names(reader), std::back_inserter(mfield_names));

    const auto num_cells = reader.number_of_cells();
    const auto num_points = reader.number_of_points();
    const auto num_all_cells = GridFormat::Parallel::broadcast(comm, GridFormat::Parallel::sum(comm, num_cells));
    const auto num_all_points = GridFormat::Parallel::broadcast(comm, GridFormat::Parallel::sum(comm, num_points));

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "pvtp_reader_name"_test = [&] () {
        expect(reader.name() == "PVTPReader");
    };

    "parallel_pvtp_read_number_of_pieces"_test = [&] () {
        expect(eq(reader.number_of_pieces(), static_cast<std::size_t>(GridFormat::Parallel::size(comm))));
    };

    // test that sequential I/O yields the expected results
    if (rank == 0) {
        reader = GridFormat::PVTPReader{};
        reader.open(test_filename);

        "sequential_pvtp_read_number_of_entities"_test = [&] () {
            expect(eq(reader.number_of_cells(), num_all_cells));
            expect(eq(reader.number_of_points(), num_all_points));
        };

        "sequential_pvtp_read_field_names"_test = [&] () {
            expect(std::ranges::equal(point_field_names(reader), pfield_names));
            expect(std::ranges::equal(cell_field_names(reader), cfield_names));
            expect(std::ranges::equal(meta_data_field_names(reader), mfield_names));
        };

        const auto sequential_grid = [&] () {
            GridFormat::Test::UnstructuredGridFactory<2, 3> factory;
            reader.export_grid(factory);
            return std::move(factory).grid();
        } ();

        "sequential_pvtp_read_point_fields"_test = [&] () {
            for (const auto& [name, field_ptr] : point_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::points(sequential_grid)
                ));
        };

        "sequential_pvtp_read_cell_fields"_test = [&] () {
            for (const auto& [name, field_ptr] : cell_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(
                    name, field_ptr, sequential_grid, GridFormat::cells(sequential_grid)
                ));
        };
    }

    MPI_Finalize();
    return 0;
}

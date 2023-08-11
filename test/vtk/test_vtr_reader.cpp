// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <iterator>
#include <ranges>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/vtr_reader.hpp>
#include <gridformat/vtk/vtr_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

#ifndef TEST_DATA_PATH
#define TEST_DATA_PATH ""
#endif


int main() {
    const GridFormat::Test::StructuredGrid<2> grid{
        {1.0, 1.0},
        {4, 5}
    };

    GridFormat::VTRWriter writer{grid};
    GridFormat::VTRReader reader;
    GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        "reader_vtr_test_file_2d_in_2d"
    );

    const std::string test_data_path_name{TEST_DATA_PATH};
    if (test_data_path_name.empty()) {
        std::cout << "No test data folder defined, skipping further tests" << std::endl;
        return 42;
    }

    if (!std::filesystem::directory_entry{std::filesystem::path{test_data_path_name}}.exists()) {
        std::cout << "Test data folder does not exist, skipping further tests" << std::endl;
        return 42;
    }

    std::vector<std::string> vtr_files;
    std::ranges::copy(
        std::filesystem::directory_iterator{std::filesystem::path{test_data_path_name}}
        | std::views::filter([] (const auto& p) { return std::filesystem::is_regular_file(p); })
        | std::views::filter([] (const std::filesystem::path& p) { return p.extension() == ".vtr"; })
        | std::views::transform([] (const std::filesystem::path& p) { return p.string(); }),
        std::back_inserter(vtr_files)
    );
    if (vtr_files.empty()) {
        std::cout << "No test vtr files found in folder " << test_data_path_name << ". Skipping..." << std::endl;
        return 42;
    }

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "vtr_reader_name"_test = [&] () {
        expect(reader.name() == "VTRReader");
    };

    "vtk_written_vtr_files"_test = [&] () {
        for (const std::string& vtr_filepath : vtr_files) {
            std::cout << "Testing '" << GridFormat::as_highlight(vtr_filepath) << "'" << std::endl;
            reader.open(vtr_filepath);

            expect(eq(reader.number_of_pieces(), std::size_t{1}));

            const auto grid = [&] () {
                GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
                reader.export_grid(factory);
                return std::move(factory).grid();
            } ();

            for (const auto& [name, field_ptr] : point_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(name, field_ptr, grid, GridFormat::points(grid)));
            for (const auto& [name, field_ptr] : cell_fields(reader))
                expect(GridFormat::Test::test_field_values<2>(name, field_ptr, grid, GridFormat::cells(grid)));
        }
    };

    return 0;
}

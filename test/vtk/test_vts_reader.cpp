// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <filesystem>
#include <iterator>
#include <ranges>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/vts_reader.hpp>
#include <gridformat/vtk/vts_writer.hpp>

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

    GridFormat::VTSWriter writer{grid};
    GridFormat::VTSReader reader;
    GridFormat::Test::test_reader<2, 2>(
        writer,
        reader,
        "reader_vts_test_file_2d_in_2d"
    );

    const std::string test_data_path_name{TEST_DATA_PATH};
    if (test_data_path_name.empty()) {
        std::cout << "No test data folder defined, skipping further tests" << std::endl;
        return 0;
    }

    if (!std::filesystem::directory_entry{std::filesystem::path{test_data_path_name}}.exists()) {
        std::cout << "Test data folder does not exist, skipping further tests" << std::endl;
        return 0;
    }

    std::vector<std::string> vts_files;
    std::ranges::copy(
        std::filesystem::directory_iterator{std::filesystem::path{test_data_path_name}}
        | std::views::filter([] (const auto& p) { return std::filesystem::is_regular_file(p); })
        | std::views::filter([] (const std::filesystem::path& p) { return p.extension() == ".vts"; })
        | std::views::transform([] (const std::filesystem::path& p) { return p.string(); }),
        std::back_inserter(vts_files)
    );
    if (vts_files.empty()) {
        std::cout << "No test vts files found in folder " << test_data_path_name << ". Skipping..." << std::endl;
        return 0;
    }

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    "vtk_written_vts_files"_test = [&] () {
        for (const std::string& vts_filepath : vts_files) {
            std::cout << "Testing '" << GridFormat::as_highlight(vts_filepath) << "'" << std::endl;
            reader.open(vts_filepath);

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

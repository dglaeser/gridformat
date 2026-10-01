// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include <gridformat/vtk/hdf_unstructured_grid_writer.hpp>
#include <gridformat/vtk/hdf_unstructured_grid_reader.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "vtk_hdf_bool_field_test"_test = [&] () {
        GridFormat::VTKHDFUnstructuredGridWriter writer{grid};
        writer.set_cell_field("true_field", [] (auto&&...) { return true; });
        writer.set_cell_field("false_field", [] (auto&&...) { return false; });
        writer.write("vtk_hdf_bool_field_test");

        GridFormat::VTKHDFUnstructuredGridReader reader;
        reader.open("vtk_hdf_bool_field_test.hdf");
        const auto true_field = reader.cell_field("true_field")->template export_to<std::vector<bool>>();
        const auto false_field = reader.cell_field("false_field")->template export_to<std::vector<bool>>();
        expect(std::ranges::all_of(true_field, [] (bool value) { return value; }));
        expect(std::ranges::all_of(false_field, [] (bool value) { return not value; }));

        std::filesystem::remove("vtk_hdf_bool_field_test.hdf");
    };

    {
        GridFormat::VTKHDFUnstructuredGridWriter writer{grid};
        {
            GridFormat::VTKHDFUnstructuredGridReader reader;
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_unstructured_test_file_2d_in_2d");
            "vtk_hdf_unstructured_grid_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), std::size_t{1}));
            };
        }
        {  // test also the convenience reader
            GridFormat::VTKHDFReader reader;
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_unstructured_test_file_2d_in_2d_from_generic");
            "vtk_hdf_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), std::size_t{1}));
            };
            "vtk_hdf_unstructured_grid_reader_name"_test  = [&] () {
                expect(reader.name().starts_with("VTKHDFUnstructuredGridReader"));
            };
        }
    }
    {
        {
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid,
                "reader_vtk_hdf_unstructured_time_series_2d_in_2d"
            };
            GridFormat::VTKHDFUnstructuredGridReader reader;
            test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, filename};
            });
            "vtk_hdf_unstructured_grid_time_series_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), std::size_t{1}));
            };
        }
        {  // test also the convenience reader
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid,
                "reader_vtk_hdf_unstructured_time_series_2d_in_2d_from_generic"
            };
            GridFormat::VTKHDFReader reader;
            test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, filename};
            });
            "vtk_hdf_time_series_reader_num_pieces"_test = [&] () {
                expect(eq(reader.number_of_pieces(), std::size_t{1}));
            };
        }
    }

    const std::filesystem::path test_data_path{TEST_DATA_PATH};
    std::vector<std::string> vtk_files;
    if (std::filesystem::exists(test_data_path))
        std::ranges::copy(
            std::filesystem::directory_iterator{test_data_path}
            | std::views::transform([] (const auto& entry) { return entry.path(); })
            | std::views::filter([] (const std::filesystem::path& p) {
                return p.extension() == ".hdf" && p.filename().string().starts_with("vtk_hdf_test_file_unstructured");
            })
            | std::views::transform([] (const std::filesystem::path& p) { return p.string(); }),
            std::back_inserter(vtk_files)
        );
    if (vtk_files.empty()) {
        std::cout << "No vtk-written test files found in " << test_data_path << ". Skipping..." << std::endl;
        return 42;
    }

    "vtk_written_vtk_hdf_unstructured_files"_test = [&] () {
        for (const auto& filename : vtk_files) {
            std::cout << "Testing '" << GridFormat::as_highlight(filename) << "'" << std::endl;
            GridFormat::VTKHDFReader reader;
            reader.open(filename);
            expect(eq(reader.number_of_pieces(), std::size_t{1}));

            const auto vtk_grid = [&] () {
                GridFormat::Test::UnstructuredGridFactory<2, 2> factory;
                reader.export_grid(factory);
                return std::move(factory).grid();
            } ();
            expect(GridFormat::Test::test_field_values<2>(
                "pscalar", reader.point_field("pscalar"), vtk_grid, GridFormat::points(vtk_grid)
            ));
            expect(GridFormat::Test::test_field_values<2>(
                "cscalar", reader.cell_field("cscalar"), vtk_grid, GridFormat::cells(vtk_grid)
            ));

            expect(eq(reader.meta_data_field("text")->template export_to<std::string>(), std::string{"some_text"}));
            expect(std::ranges::equal(
                reader.meta_data_field("numbers")->template export_to<std::vector<int>>(),
                std::vector<int>{1, 2, 3, 4}
            ));
        }
    };

    return 0;
}

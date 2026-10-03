// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>
#include <cmath>
#include <string>
#include <iostream>
#include <algorithm>
#include <filesystem>

#include <gridformat/vtk/hdf_unstructured_grid_writer.hpp>
#include <gridformat/vtk/hdf_image_grid_writer.hpp>
#include <gridformat/vtk/hdf_image_grid_reader.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/structured_grid.hpp"
#include "../reader_tests.hpp"
#include "../testing.hpp"


template<typename Reader>
void test(Reader&& reader, const std::string& suffix = "") {
    const GridFormat::Test::StructuredGrid<3> grid{{1.0, 1.0, 1.0}, {4, 5, 6}};
    GridFormat::VTKHDFImageGridWriter writer{grid};

    test_reader<3, 3>(writer, reader, "reader_vtk_hdf_structured_image_test_file_3d_in_3d" + suffix);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    const auto spacing = reader.spacing();
    const auto extents = reader.extents();

    "vtk_hdf_image_grid_reader"_test  = [&] () {
        expect(eq(reader.number_of_pieces(), std::size_t{1}));
    };

    "vtk_hdf_image_grid_reader_name"_test  = [&] () {
        expect(reader.name().starts_with("VTKHDFImageGridReader"));
    };

    "vtk_hdf_image_grid_reader_spacing"_test = [&] () {
        expect(std::abs(spacing[0] - 1.0/4.0) < 1e-6);
        expect(std::abs(spacing[1] - 1.0/5.0) < 1e-6);
    };

    "vtk_hdf_image_grid_reader_extents"_test = [&] () {
        expect(eq(extents[0], std::size_t{4}));
        expect(eq(extents[1], std::size_t{5}));
    };

    "vtk_hdf_image_grid_reader_point_field"_test = [&] () {
        GridFormat::Test::StructuredGrid<3> grid_in{
            {spacing[0]*extents[0], spacing[1]*extents[1], spacing[2]*extents[2]},
            {extents[0], extents[1], extents[2]},
            {0.0, 0.0, 0.0},
            false // do not shuffle, vtk file is "ordered"
        };

        std::vector<double> pscalar(reader.number_of_points(), 0.);
        reader.point_field("pscalar")->export_to(pscalar);

        std::size_t i = 0;
        for (const auto& point : GridFormat::points(grid_in)) {
            const auto read_value = pscalar[i++];
            const auto expected_value = GridFormat::Test::test_function<double>(
                GridFormat::Test::evaluation_position(grid_in, point)
            );
            expect(std::abs(read_value - expected_value) < 1e-6);
        }
    };

    {  // test time series as well
        GridFormat::VTKHDFImageGridTimeSeriesWriter writer{
            grid,
            "reader_vtk_hdf_structured_time_series_image_3d_in_3d" + suffix
        };
        test_reader<3, 3>(writer, reader, [] (const auto& grid, const auto& filename) {
            return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, filename};
        });
    }
}


int main() {
    test(GridFormat::VTKHDFImageGridReader{});
    test(GridFormat::VTKHDFReader{}, "_from_generic");

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    // the values differ per step, such that reading the wrong step goes not unnoticed
    const auto numbers_at = [] (int step) { return std::vector<int>{step, step + 1, step + 2}; };
    const auto write_transient = [&] (const std::string& base, bool static_meta_data) {
        const GridFormat::Test::StructuredGrid<3> grid{{1.0, 1.0, 1.0}, {2, 2, 2}};
        GridFormat::VTKHDFImageGridTimeSeriesWriter writer{grid, base, {.static_grid = true, .static_meta_data = static_meta_data}};
        for (int step : {0, 1, 2}) {
            writer.set_meta_data("numbers", numbers_at(step));
            writer.write(static_cast<double>(step));
        }
        return base + ".hdf";
    };
    const auto check_numbers = [&] (const std::string& filename, auto&& expected_at) {
        GridFormat::VTKHDFImageGridReader reader;
        reader.open(filename);
        expect(eq(reader.number_of_steps(), std::size_t{3}));
        for (std::size_t step = 0; step < reader.number_of_steps(); ++step) {
            reader.set_step(step);
            expect(std::ranges::equal(
                reader.meta_data_field("numbers")->template export_to<std::vector<int>>(),
                expected_at(static_cast<int>(step))
            ));
        }
    };

    "vtk_hdf_image_time_series_static_meta_data"_test = [&] () {
        const auto filename = write_transient("reader_vtk_hdf_image_static_meta_data", true);
        check_numbers(filename, [&] (int) { return numbers_at(0); });
    };

    "vtk_hdf_image_time_series_legacy_meta_data_layout"_test = [&] () {
        const auto filename = write_transient("reader_vtk_hdf_image_legacy_meta_data", false);
        {   // GridFormat <= 0.5 stored one row per step, with an additional dimension: (num_steps, 1, N),
            // and the step index as offset, without sizes
            HighFive::File file{filename, HighFive::File::ReadWrite};
            std::vector<std::vector<std::vector<int>>> legacy;
            for (int step : {0, 1, 2})
                legacy.push_back({numbers_at(step)});
            auto group = file.getGroup("/VTKHDF/FieldData");
            group.unlink("numbers");
            group.createDataSet("numbers", legacy);
            file.getGroup("/VTKHDF/Steps/FieldDataSizes").unlink("numbers");
            auto offsets = file.getGroup("/VTKHDF/Steps/FieldDataOffsets");
            offsets.unlink("numbers");
            offsets.createDataSet("numbers", std::vector<std::size_t>{0, 1, 2});
        }
        check_numbers(filename, numbers_at);
    };

    const std::filesystem::path test_data_path{TEST_DATA_PATH};
    std::vector<std::string> vtk_files;
    if (std::filesystem::exists(test_data_path))
        std::ranges::copy(
            std::filesystem::directory_iterator{test_data_path}
            | std::views::transform([] (const auto& entry) { return entry.path(); })
            | std::views::filter([] (const std::filesystem::path& p) {
                return p.extension() == ".hdf" && p.filename().string().starts_with("vtk_hdf_test_file_image");
            })
            | std::views::transform([] (const std::filesystem::path& p) { return p.string(); }),
            std::back_inserter(vtk_files)
        );
    if (vtk_files.empty()) {
        std::cout << "No vtk-written test files found in " << test_data_path << ". Skipping..." << std::endl;
        return 42;
    }

    "vtk_written_vtk_hdf_image_files"_test = [&] () {
        for (const auto& filename : vtk_files) {
            std::cout << "Testing '" << GridFormat::as_highlight(filename) << "'" << std::endl;
            GridFormat::VTKHDFReader reader;
            reader.open(filename);
            expect(eq(reader.number_of_pieces(), std::size_t{1}));

            const auto vtk_grid = [&] () {
                GridFormat::Test::UnstructuredGridFactory<3, 3> factory;
                reader.export_grid(factory);
                return std::move(factory).grid();
            } ();
            expect(eq(GridFormat::number_of_cells(vtk_grid), std::size_t{5*5*3}));
            expect(GridFormat::Test::test_field_values<3>(
                "pscalar", reader.point_field("pscalar"), vtk_grid, GridFormat::points(vtk_grid)
            ));
            expect(GridFormat::Test::test_field_values<3>(
                "cscalar", reader.cell_field("cscalar"), vtk_grid, GridFormat::cells(vtk_grid)
            ));
        }
    };

    return 0;
}

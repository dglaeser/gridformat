// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>
#include <cmath>

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

    // TODO: Test cell&field data once a new VTK version is released that fixes issues
    test_reader<3, 3>(writer, reader, "reader_vtk_hdf_structured_image_test_file_3d_in_3d" + suffix, {
        .write_cell_data = false,
        .write_meta_data = false
    });

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
        // TODO: use filenames that include these in the regression tests once the VTK fixes are available
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
    return 0;
}

// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/common/hdf5.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"

int main() {
    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    {
        const auto grid = GridFormat::Test::make_unstructured<2, 2>();
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid,
            "vtk_hdf_time_series_2d_in_2d_unstructured",
            GridFormat::VTK::HDFTransientOptions{
                .static_grid = false,
                .static_meta_data = false
            }
        };
        GridFormat::Test::write_test_time_series<2>(writer);

        "hdf_time_series_steps_dimensions"_test = [&] () {
            GridFormat::HDF5::File file{"vtk_hdf_time_series_2d_in_2d_unstructured.hdf"};
            expect(eq(
                file.get_dimensions("/VTKHDF/FieldData/literal").value().at(0),
                5_ul
            ));
            expect(eq(
                file.get_dimensions("/VTKHDF/Steps/CellOffsets").value().at(0),
                5_ul
            ));

            auto cell_offsets = file.read_dataset_to<std::vector<std::size_t>>("/VTKHDF/Steps/CellOffsets");
            auto lit_offsets = file.read_dataset_to<std::vector<std::size_t>>("/VTKHDF/Steps/FieldDataOffsets/literal");
            GridFormat::Ranges::sort_and_unique(cell_offsets);
            GridFormat::Ranges::sort_and_unique(lit_offsets);
            expect(eq(cell_offsets.size(), 5_ul));
            expect(eq(lit_offsets.size(), 5_ul));
        };
    }

    {  // test with static grid and meta data
        const auto grid = GridFormat::Test::make_unstructured<2, 2>();
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid,
            "vtk_hdf_time_series_2d_in_2d_unstructured_static_grid",
            GridFormat::VTK::HDFTransientOptions{
                .static_grid = true,
                .static_meta_data = true
            }
        };
        GridFormat::Test::write_test_time_series<2>(writer);

        "hdf_time_series_static_grid_steps_dimensions"_test = [&] () {
            GridFormat::HDF5::File file{"vtk_hdf_time_series_2d_in_2d_unstructured_static_grid.hdf"};
            expect(eq(
                file.get_dimensions("/VTKHDF/FieldData/literal").value().at(0),
                1_ul
            ));
            expect(eq(
                file.get_dimensions("/VTKHDF/Steps/CellOffsets").value().at(0),
                5_ul
            ));

            auto cell_offsets = file.read_dataset_to<std::vector<std::size_t>>("/VTKHDF/Steps/CellOffsets");
            auto lit_offsets = file.read_dataset_to<std::vector<std::size_t>>("/VTKHDF/Steps/FieldDataOffsets/literal");
            expect(eq(cell_offsets.size(), 5_ul));
            expect(eq(lit_offsets.size(), 5_ul));
            expect(std::ranges::equal(cell_offsets, std::vector{0, 0, 0, 0, 0}));
            expect(std::ranges::equal(lit_offsets, std::vector{0, 0, 0, 0, 0}));
        };
    }

    {
        const GridFormat::Test::StructuredGrid<2> grid{
            {1.0, 1.0},
            {5, 7}
        };
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid,
            "vtk_hdf_time_series_2d_in_2d_image"
        };
        GridFormat::Test::write_test_time_series<2>(writer);
    }

    return 0;
}

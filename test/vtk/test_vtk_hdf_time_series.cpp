// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#include <highfive/H5Easy.hpp>
#pragma GCC diagnostic pop

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
        GridFormat::Test::add_meta_data(writer);
        for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
            const auto test_data = GridFormat::Test::make_test_data<2, double>(grid, t);
            GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
            std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(t)) << "'" << std::endl;
        }

        "hdf_time_series_steps_dimensions"_test = [&] () {
            HighFive::File file{
                "vtk_hdf_time_series_2d_in_2d_unstructured.hdf",
                HighFive::File::ReadOnly
            };
            expect(eq(
                file.getGroup("/VTKHDF/FieldData").getDataSet("literal").getDimensions().at(0),
                5_ul
            ));
            expect(eq(
                file.getGroup("/VTKHDF/Steps").getDataSet("CellOffsets").getDimensions().at(0),
                5_ul
            ));

            // offsets should be increasing with "time"
            std::vector<std::size_t> cell_offsets;
            std::vector<std::size_t> field_data_offsets;
            file.getGroup("/VTKHDF/Steps").getDataSet("CellOffsets").read(cell_offsets);
            file.getGroup("/VTKHDF/Steps/FieldDataOffsets").getDataSet("literal").read(field_data_offsets);
            auto copy = cell_offsets; GridFormat::Ranges::sort_and_unique(copy);
            expect(std::ranges::equal(cell_offsets, copy));
            copy = field_data_offsets; GridFormat::Ranges::sort_and_unique(copy);
            expect(std::ranges::equal(field_data_offsets, copy));
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
        GridFormat::Test::add_meta_data(writer);
        for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
            const auto test_data = GridFormat::Test::make_test_data<2, double>(grid, t);
            GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
            std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(t)) << "'" << std::endl;
        }

        "hdf_time_series_static_grid_steps_dimensions"_test = [&] () {
            HighFive::File file{
                "vtk_hdf_time_series_2d_in_2d_unstructured_static_grid.hdf",
                HighFive::File::ReadOnly
            };
            expect(eq(
                file.getGroup("/VTKHDF/FieldData").getDataSet("literal").getDimensions().at(0),
                1_ul
            ));
            expect(eq(
                file.getGroup("/VTKHDF/Steps").getDataSet("CellOffsets").getDimensions().at(0),
                5_ul
            ));

            // offsets all be zero
            std::vector<std::size_t> cell_offsets;
            std::vector<std::size_t> field_data_offsets;
            file.getGroup("/VTKHDF/Steps").getDataSet("CellOffsets").read(cell_offsets);
            file.getGroup("/VTKHDF/Steps/FieldDataOffsets").getDataSet("literal").read(field_data_offsets);
            expect(std::ranges::equal(cell_offsets, std::vector{0, 0, 0, 0, 0}));
            expect(std::ranges::equal(field_data_offsets, std::vector{0, 0, 0, 0, 0}));
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
        GridFormat::Test::add_meta_data(writer);
        for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
            const auto test_data = GridFormat::Test::make_test_data<2, double>(grid, t);
            GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
            std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(t)) << "'" << std::endl;
        }
    }

    return 0;
}

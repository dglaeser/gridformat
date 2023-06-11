// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"

int main() {
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

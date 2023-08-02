// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/xml_time_series_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main() {
    const auto grid = GridFormat::Test::make_unstructured<2, 2>();
    GridFormat::VTKXMLTimeSeriesWriter writer{
        GridFormat::VTUWriter{grid},
        "vtu_time_series_2d_in_2d"
    };
    GridFormat::Test::write_test_time_series<2>(writer);
    return 0;
}

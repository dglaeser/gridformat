// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/common/logging.hpp>
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

    GridFormat::Test::add_meta_data(writer);
    for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const auto test_data = GridFormat::Test::make_test_data<2, double>(grid, t);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
        std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(t)) << "'" << std::endl;
    }

    return 0;
}

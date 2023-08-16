// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <stdexcept>
#include <iostream>
#include <array>
#include <vector>
#include <cmath>

#include <gridformat/gridformat.hpp>

double function(const std::array<double, 2>& x) {
    return x[0]*x[1];
}

int main () {
    // The grid we want to use for discretization/visualization
    GridFormat::ImageGrid<2, double> grid{
        {1.0, 1.0}, // domain size
        {10, 12}    // number of cells (pixels) in each direction
    };

    // This shows the `GridFormat` API: Construct a writer for the desired format,
    // and add point/cell fields as lambdas.
    GridFormat::Writer writer{GridFormat::vtu, grid};
    writer.set_point_field("point_field", [&] (const auto& point) {
        return function(grid.position(point));
    });
    writer.set_cell_field("cell_field", [&] (const auto& cell) {
        return function(grid.center(cell));
    });
    const auto written_file = writer.write("analytical"); // extension is added by the writer
    std::cout << "Wrote '" << written_file << "'" << std::endl;

    // Read the data back in (if you omit the format specifier in the reader constructor, it
    // will try to deduce the format from the file and select an appropriate reader automatically)
    GridFormat::Reader reader{GridFormat::vtu};
    reader.open(written_file);
    std::vector<double> cell_field_values(reader.number_of_cells());
    std::vector<double> point_field_values(reader.number_of_points());
    reader.cell_field("cell_field")->export_to(cell_field_values);
    reader.point_field("point_field")->export_to(point_field_values);

    // Let's verify that the values match with our function
    std::size_t point_index = 0;
    for (const auto& point : grid.points()) {
        const auto function_value = function(grid.position(point));
        const auto read_value = point_field_values[point_index++];
        if (std::abs(function_value - read_value) > 1e-6)
            throw std::runtime_error("Point field value deviation");
    }

    std::size_t cell_index = 0;
    for (const auto& cell : grid.cells()) {
        const auto function_value = function(grid.center(cell));
        const auto read_value = cell_field_values[cell_index++];
        if (std::abs(function_value - read_value) > 1e-6)
            throw std::runtime_error("Cell field value deviation");
    }
    std::cout << "Successfully tested the read point/cell values" << std::endl;

    return 0;
}

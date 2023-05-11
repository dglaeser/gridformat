// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>

#include <gridformat/common/concepts.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "triangulation.hpp"

inline double test_function(const std::array<double, 2>& position) {
    return std::sin(position[0])*std::cos(position[1]);
}

int main() {
    GridFormatExamples::Triangulation grid{
        {
            {{0.0, 0.0}},
            {{1.0, 0.0}},
            {{0.0, 1.0}},
            {{1.0, 1.0}}
        },
        {
            {{0, 1, 2}},
            {{1, 2, 3}}
        }
    };

    auto writer = GridFormat::VTKHDFWriter{grid};
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return test_function(vertex.position);
    });
    writer.set_cell_field("cfunc", [&] (const auto& cell) {
        return test_function(grid.center(cell));
    });
    writer.write("vtk_hdf_unstructured");

    return 0;
}

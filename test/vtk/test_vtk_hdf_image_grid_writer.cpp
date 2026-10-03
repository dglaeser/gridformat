// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <numbers>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"

template<typename Grid>
void _test(Grid&& grid, const std::string& filename) {
    GridFormat::VTKHDFWriter writer{grid};
    GridFormat::Test::write_test_file<GridFormat::dimension<Grid>>(
        writer,
        filename);
}

int main() {
    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            _test(
                GridFormat::Test::StructuredGrid<2>({{1.0, 1.0}}, {{nx, ny}}),
                std::string{"vtk_hdf_image_2d_in_2d"}
                    + "_" + std::to_string(nx)
                    + "_" + std::to_string(ny)
            );

    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            for (std::size_t nz : {2, 4})
                _test(
                    GridFormat::Test::StructuredGrid<3>({{1.0, 1.0, 1.0}}, {{nx, ny, nz}}),
                    std::string{"vtk_hdf_image_3d_in_3d"}
                    + "_" + std::to_string(nx)
                    + "_" + std::to_string(ny)
                    + "_" + std::to_string(nz)
                );

    constexpr auto sqrt2_half = 1.0/std::numbers::sqrt2;
    _test(
        GridFormat::Test::OrientedStructuredGrid<2>{
            {
                std::array<double, 2>{sqrt2_half, sqrt2_half},
                std::array<double, 2>{-sqrt2_half, sqrt2_half}
            },
            {{1.0, 1.0}},
            {{3, 4}}
        },
        "vtk_hdf_image_2d_in_2d_oriented"
    );

    _test(
        GridFormat::Test::OrientedStructuredGrid<3>{
            {
                std::array<double, 3>{sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{-sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{0.0, 0.0, 1.0}
            },
            {{1.0, 1.0, 1.0}},
            {{2, 3, 4}}
        },
        "vtk_hdf_image_3d_in_3d_oriented"
    );

    return 0;
}

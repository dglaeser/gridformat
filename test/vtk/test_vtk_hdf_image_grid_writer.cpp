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
    // TODO: There is a (fixed) issue in the vtkHDFReader when reading cell arrays from image grids:
    //       see https://gitlab.kitware.com/vtk/vtk/-/issues/18860
    //       Once this is in a release version we should also add cell data
    // TODO: There is an issue with field data (https://gitlab.kitware.com/vtk/vtk/-/issues/19030)
    //       Once fixed, add meta data as well
    GridFormat::VTKHDFWriter writer{grid};
    GridFormat::Test::write_test_file<GridFormat::dimension<Grid>>(
        writer,
        filename,
        {
            .write_cell_data = false,
            .write_meta_data = false
        }
    );
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

    // TODO: the vtkHDFReader in python, at least the way we use it, does not yield the correct
    //       point coordinates, but still the axis-aligned ones. Interestingly, ParaView correctly
    //       displays the files we produce. Also, we obtain the points of a read .vti files in the
    //       same way in our test script and that works fine. For now, we only test if the files
    //       are successfully written, but we use filenames such that they are not regression-tested.
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
        "_ignore_regression_vtk_2d_in_2d_image_oriented"
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
        "_ignore_regression_vtk_3d_in_3d_image_oriented"
    );

    return 0;
}

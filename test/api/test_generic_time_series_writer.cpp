// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>

#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"

template<typename Writer>
void write(Writer&& writer) {
    const auto& grid = writer.grid();
    GridFormat::Test::add_meta_data(writer);
    for (double sim_time : {0.0, 0.5, 1.0}) {
        writer.set_point_field("point_func", [&] (const auto& p) {
            return GridFormat::Test::test_function<double>(grid.position(p))*sim_time;
        });
        writer.set_cell_field("cell_func", [&] (const auto& c) {
            return GridFormat::Test::test_function<double>(grid.center(c))*sim_time;
        });
        std::cout << "Writing at t = " << sim_time << std::endl;
        std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(sim_time)) << "'" << std::endl;
    }
}

int main() {
    using GridFormat::Test::test_function;

    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {4, 5}};
    write(GridFormat::Writer{GridFormat::pvd, grid, "generic_time_series_2d_in_2d_default"});
    write(GridFormat::Writer{GridFormat::pvd_with(GridFormat::vtu), grid, "generic_time_series_2d_in_2d_vtu"});
    write(GridFormat::Writer{GridFormat::pvd_with(GridFormat::vti), grid, "generic_time_series_2d_in_2d_vti"});
    write(GridFormat::Writer{GridFormat::pvd_with(GridFormat::vtr), grid, "generic_time_series_2d_in_2d_vtr"});
    write(GridFormat::Writer{GridFormat::pvd_with(GridFormat::vts({.encoder = GridFormat::Encoding::ascii})), grid, "generic_time_series_2d_in_2d_vts"});
    // add pvd to make the regression script include the files (see CMakeLists.txt)
    write(GridFormat::Writer{GridFormat::time_series(GridFormat::vtu), grid, "generic_time_series_2d_in_2d_pvd"});

#if GRIDFORMAT_HAVE_HIGH_FIVE
    // TODO: include in regression test-suite once new VTK version is published
    write(GridFormat::Writer{
        GridFormat::time_series(GridFormat::vtk_hdf),
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d"
    });
    write(GridFormat::Writer{
        GridFormat::time_series(GridFormat::FileFormat::VTKHDFImage{}),
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d_image"
    });
    write(GridFormat::Writer{
        GridFormat::time_series(GridFormat::FileFormat::VTKHDFUnstructured{}),
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d_unstructured_explicit"
    });

    write(GridFormat::Writer{
        GridFormat::vtk_hdf_transient,
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d_transient_explicit"
    });
    write(GridFormat::Writer{
        GridFormat::FileFormat::VTKHDFImageTransient{},
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d_transient_image_explicit"
    });
    write(GridFormat::Writer{
        GridFormat::FileFormat::VTKHDFUnstructuredTransient{},
        grid,
        "_ignore_regression_generic_time_series_2d_in_2d_transient_unstructured_explicit"
    });
#endif
    return 0;
}

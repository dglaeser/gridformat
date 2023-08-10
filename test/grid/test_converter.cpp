// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <iostream>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/converter.hpp>

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "unstructured_grid.hpp"
#include "structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"


int main() {
    const GridFormat::Test::StructuredGrid<3> test_grid{{1.0, 1.0, 1.0}, {4, 5, 6}};

    // TODO: once the vtk readers are fixed (should be VTK 9.2.7), test also field/cell data
    GridFormat::VTKHDFWriter test_writer{test_grid};
    const auto test_data = GridFormat::Test::make_test_data<3>(test_grid, GridFormat::float64);
    GridFormat::Test::add_test_point_data(test_writer, test_data, GridFormat::float32);
    const auto test_filename = test_writer.write("converter_test_file_vtk_hdf_3d_in_3d_in");
    std::cout << "Wrote '" << GridFormat::as_highlight(test_filename) << "'" << std::endl;

    GridFormat::VTKHDFReader reader;
    reader.open(test_filename);

    std::cout << "Converting image to unstructured vtk hdf file" << std::endl;
    GridFormat::convert(reader, "converter_test_fileunstructured_vtk_hdf_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTKHDFUnstructuredGridWriter{grid};
    });

    std::cout << "\"Converting\" image to structured vtk hdf file" << std::endl;
    GridFormat::convert(reader, "converter_test_fileimage_vtk_hdf_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTKHDFImageGridWriter{grid};
    });

    std::cout << "\"Converting\" image to vti file" << std::endl;
    GridFormat::convert(reader, "converter_test_filevti_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTIWriter{grid};
    });

    std::cout << "\"Converting\" image to vtu file" << std::endl;
    GridFormat::convert(reader, "converter_test_filevtu_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTUWriter{grid};
    });

    // Write & convert a test time series
    GridFormat::VTKHDFTimeSeriesWriter test_ts_writer{test_grid, "converter_time_series_vtk_hdf_3d_in_3d_in"};
    const auto test_ts_filename = [&] () {
        std::string ts_filename;
        for (std::size_t i = 0; i < 5; ++i) {
            const auto time_step = static_cast<double>(i)*0.2;
            const auto test_data = GridFormat::Test::make_test_data<3>(test_grid, GridFormat::float64, time_step);
            GridFormat::Test::add_test_point_data(test_ts_writer, test_data, GridFormat::float32);
            ts_filename = test_ts_writer.write(time_step);
        }
        return ts_filename;
    } ();

    std::cout << "Converting vtk hdf time series to .pvd" << std::endl;
    reader.open(test_ts_filename);
    GridFormat::convert(reader, [&] (const auto& grid) {
        return GridFormat::PVDWriter{GridFormat::VTUWriter{grid}, "converter_time_series_vtk_hdf_3d_in_3d"};
    });

    return 0;
}

// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/hdf_unstructured_grid_writer.hpp>
#include <gridformat/vtk/hdf_unstructured_grid_reader.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "../reader_tests.hpp"

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    {
        GridFormat::VTKHDFUnstructuredGridWriter writer{grid};
        {
            GridFormat::VTKHDFUnstructuredGridReader reader;
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_unstructured_test_file_2d_in_2d");
        }
        {  // test also the convenience reader
            GridFormat::VTKHDFReader reader;
            test_reader<2, 2>(writer, reader, "reader_vtk_hdf_unstructured_test_file_2d_in_2d_from_generic");
        }
    }
    {
        {
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid,
                "reader_vtk_hdf_unstructured_time_series_2d_in_2d"
            };
            GridFormat::VTKHDFUnstructuredGridReader reader;
            test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, filename};
            });
        }
        {  // test also the convenience reader
            GridFormat::VTKHDFUnstructuredTimeSeriesWriter writer{
                grid,
                "reader_vtk_hdf_unstructured_time_series_2d_in_2d_from_generic"
            };
            GridFormat::VTKHDFReader reader;
            test_reader<2, 2>(writer, reader, [] (const auto& grid, const auto& filename) {
                return GridFormat::VTKHDFUnstructuredTimeSeriesWriter{grid, filename};
            });
        }
    }
    return 0;
}

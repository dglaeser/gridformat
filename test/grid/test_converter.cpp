// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <iostream>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/converter.hpp>

#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/hdf_reader.hpp>

#include "unstructured_grid.hpp"
#include "structured_grid.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"


int main() {
    const GridFormat::Test::StructuredGrid<3> struct_grid{{1.0, 1.0, 1.0}, {4, 5, 6}};
    const auto struct_data = GridFormat::Test::make_test_data<3>(struct_grid, GridFormat::float64);
    GridFormat::VTKHDFWriter struct_writer{struct_grid};

    // TODO: once the vtk readers are fixed (should be VTK 9.2.7), test also field/cell data
    GridFormat::Test::add_test_point_data(struct_writer, struct_data, GridFormat::float32);
    const auto struct_filename = struct_writer.write("converter_image_vtk_hdf_3d_in_3d_in");
    std::cout << "Wrote '" << GridFormat::as_highlight(struct_filename) << "'" << std::endl;

    GridFormat::VTKHDFReader reader;
    reader.open(struct_filename);

    std::cout << "Converting image to unstructured vtk hdf file" << std::endl;
    GridFormat::convert(reader, "converter_unstructured_vtk_hdf_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTKHDFUnstructuredGridWriter{grid};
    });

    std::cout << "\"Converting\" image to structured vtk hdf file" << std::endl;
    GridFormat::convert(reader, "converter_image_vtk_hdf_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTKHDFImageGridWriter{grid};
    });

    std::cout << "\"Converting\" image to vti file" << std::endl;
    GridFormat::convert(reader, "converter_vti_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTIWriter{grid};
    });

    std::cout << "\"Converting\" image to vtu file" << std::endl;
    GridFormat::convert(reader, "converter_vtu_3d_in_3d_out", [&] (const auto& grid) {
        return GridFormat::VTUWriter{grid};
    });

    return 0;
}

// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();

    GridFormat::VTKHDFWriter writer{grid};
    auto test_data = GridFormat::Test::make_test_data<2, double>(grid);
    GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
    GridFormat::Test::VTK::add_meta_data(writer);
    writer.write("vtk_2d_in_2d_unstructured");

    return 0;
}

// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

template<int dim, int space_dim>
void _test() {
    GridFormat::Test::VTK::WriterTester tester{
        GridFormat::Test::make_unstructured<dim, space_dim>(),
        ".vtu"
    };
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTUWriter{grid, xml_opts};
    });
}

int main() {
    _test<0, 1>();
    _test<0, 2>();
    _test<0, 3>();

    _test<1, 1>();
    _test<1, 2>();
    _test<1, 3>();

    _test<2, 2>();
    _test<2, 3>();

    _test<3, 3>();

    // write out a structured grid as unstructured grid
    {
        GridFormat::Test::StructuredGrid<2> grid{{1.0, 1.0}, {10, 10}};
        GridFormat::VTUWriter writer{grid};
        GridFormat::Test::write_test_file<2>(writer, "vtu_2d_in_2d_from_structured_grid");
    }

    {
        GridFormat::Test::StructuredGrid<3> grid{{1.0, 1.0, 1.0}, {3, 3, 3}};
        GridFormat::VTUWriter writer{grid};
        GridFormat::Test::write_test_file<3>(writer, "vtu_3d_in_3d_from_structured_grid");
    }

    return 0;
}

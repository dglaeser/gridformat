// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/vtp_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "vtk_writer_tester.hpp"

template<int dim, int space_dim>
void _test() {
    GridFormat::Test::VTK::WriterTester tester{
        GridFormat::Test::make_unstructured<dim, space_dim>(),
        ".vtp"
    };
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTPWriter{grid, xml_opts};
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

    return 0;
}

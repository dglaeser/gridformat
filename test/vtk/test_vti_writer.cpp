// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/vtk/vti_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

template<int dim>
void _test(GridFormat::Test::StructuredGrid<dim>&& grid) {
    GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".vti"};
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTIWriter{grid, xml_opts};
    });
}

int main() {
    _test(
        GridFormat::Test::StructuredGrid<2>{
            {{1.0, 1.0}},
            {{10, 10}}
        }
    );

    _test(
        GridFormat::Test::StructuredGrid<3>{
            {{1.0, 1.0, 1.0}},
            {{10, 10, 10}}
        }
    );

    return 0;
}

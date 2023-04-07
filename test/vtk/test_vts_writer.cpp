// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <algorithm>

#include <gridformat/vtk/vts_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "vtk_writer_tester.hpp"

template<int dim>
void _test(const std::array<std::size_t, dim>& cells, bool invert = false) {
    std::array<double, dim> size;
    std::ranges::fill(size, 1.0);
    auto grid = GridFormat::Test::StructuredGrid<dim>{size, cells};
    if (invert)
        grid.invert();
    GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".vts", true, (invert ? "inverted" : "")};
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::VTSWriter{grid, xml_opts};
    });
}

int main() {
    _test<2>({10, 10});
    _test<3>({10, 10, 10});
    _test<3>({10, 10, 10}, true);
    return 0;
}

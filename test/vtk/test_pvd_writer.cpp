// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<std::size_t dim, typename Grid>
void test(const Grid& grid) {
    GridFormat::PVDWriter pvd_writer{GridFormat::VTUWriter{grid}, "pvd_time_series_2d_in_2d"};
    GridFormat::Test::write_test_time_series<dim>(pvd_writer);
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    test<2>(grid);
    return 0;
}

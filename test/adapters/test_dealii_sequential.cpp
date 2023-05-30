// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>

#include <gridformat/grid/adapters/dealii.hpp>
#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include "../make_test_data.hpp"

template<int dim, typename T>
auto as_array(const dealii::Point<dim, T>& p) {
    std::array<T, dim> result;
    for (int i = 0; i < dim; ++i)
        result[i] = p[i];
    return result;
}


template<int dim, int space_dim>
void write() {
    dealii::Triangulation<dim, space_dim> grid;
    dealii::GridGenerator::hyper_cube(grid);
    grid.refine_global(3);

    GridFormat::VTUWriter writer{grid};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(as_array(vertex.center()));
    });
    writer.set_cell_field("cfunc", [&] (const auto& cell) {
        return GridFormat::Test::test_function<double>(as_array(cell.center()));
    });
    std::cout << "Wrote '" << GridFormat::as_highlight(
        writer.write("dealii_vtu_" + std::to_string(dim) + "d_in_" + std::to_string(space_dim) + "d")
    ) << "'" << std::endl;
}

int main() {
    write<2, 2>();
    write<2, 3>();
    write<3, 3>();

    return 0;
}

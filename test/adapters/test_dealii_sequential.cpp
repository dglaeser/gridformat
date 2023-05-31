// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <iostream>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_in.h>

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

template<int dim, int space_dim, typename Grid>
void write(const Grid& grid, const std::string& suffix = "") {
    GridFormat::VTUWriter writer{grid};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(as_array(vertex.center()));
    });
    writer.set_cell_field("cfunc", [&] (const auto& cell) {
        return GridFormat::Test::test_function<double>(as_array(cell.center()));
    });
    std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(
        "dealii_vtu_" + (suffix.empty() ? "" : suffix + "_")
                      + std::to_string(dim) + "d_in_"
                      + std::to_string(space_dim) + "d"
    )) << "'" << std::endl;
}

template<int dim, int space_dim>
void write_cubes() {
    dealii::Triangulation<dim, space_dim> grid;
    dealii::GridGenerator::hyper_cube(grid);
    grid.refine_global(3);
    write<dim, space_dim>(grid);
}

template<int dim, int space_dim>
void write_simplices(const std::string& msh_filename) {
    std::cout << "Reading from mesh file '" << GridFormat::as_highlight(msh_filename) << "'" << std::endl;
    dealii::Triangulation<dim, space_dim> grid;
    dealii::GridIn<dim, space_dim>(grid).read(msh_filename);
    write<dim, space_dim>(grid, "simplices");
}

int main() {
    write_cubes<2, 2>();
    write_cubes<2, 3>();
    write_cubes<3, 3>();

    // mesh file paths are defined in CMakeLists.txt
    write_simplices<2, 2>(MESH_FILE_2D);
    write_simplices<2, 3>(MESH_FILE_2D);
    write_simplices<3, 3>(MESH_FILE_3D);

    return 0;
}

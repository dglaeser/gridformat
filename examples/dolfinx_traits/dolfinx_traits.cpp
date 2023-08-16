// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <cmath>
#include <memory>
#include <numbers>

#include <mpi.h>
#include <dolfinx.h>

#include <gridformat/traits/dolfinx.hpp>
#include <gridformat/gridformat.hpp>

using dolfinx::mesh::Mesh;
using dolfinx::mesh::CellType;

// create a hexahedral dolfinx mesh on the unit cube
auto make_mesh() {
    return std::make_shared<Mesh>(dolfinx::mesh::create_box(
        MPI_COMM_WORLD,
        {
            std::array{0., 0., 0.},
            std::array{1., 1., 1.}
        },
        {50, 50, 50},
        CellType::hexahedron
    ));
}

// Create a function space on the given mesh with the given block size (1 for scalars, 2/3 for 2d/3d vectors) and order
auto make_function_space(std::shared_ptr<Mesh> mesh, int block_size, int order) {
    return std::make_shared<const dolfinx::fem::FunctionSpace>(dolfinx::fem::create_functionspace(
        mesh,
        basix::create_element(
            basix::element::family::P,
            dolfinx::mesh::cell_type_to_basix_type(dolfinx::mesh::CellType::hexahedron),
            order,
            basix::element::lagrange_variant::unset,
            basix::element::dpc_variant::unset,
            (order == 0 ? true : false)  // discontinuous ?
        ),
        block_size
    ));
}

// Create a function on the given mesh, and simply interpolate an analytical function on it
auto make_function(std::shared_ptr<Mesh> mesh, int block_size, int order, const std::string& name) {
    static constexpr double pi2 = std::numbers::pi*2.0;
    dolfinx::fem::Function<double> function{make_function_space(mesh, block_size, order)};
    function.interpolate([&] (auto x) {
        const auto n_points = x.extent(1);
        std::vector<double> data(n_points*block_size, 0.0);
        for (int c = 0; c < block_size; ++c)
            for (unsigned int i = 0; i < n_points; ++i)
                data[n_points*c + i] = std::sin(pi2*x(0, i))
                                        *std::cos(pi2*x(1, i))
                                        *std::sin(x(2, i))
                                        *(0.5 + static_cast<double>(c));

        const auto shape = block_size > 1 ? std::vector<std::size_t>{static_cast<std::size_t>(block_size), n_points}
                                          : std::vector{n_points};
        return std::make_pair(data, shape);
    });
    function.name = name;
    return function;
}


void run_fake_simulation() {
    auto mesh = make_mesh();

    // instead of an actual simulation, we just create functions from an analytical expression
    std::cout << "Creating functions" << std::endl;
    auto scalar_nodal_function = make_function(mesh, 1, 2, "scalar_nodal_function");
    auto vector_nodal_function = make_function(mesh, 3, 2, "vector_nodal_function");
    auto scalar_cell_function = make_function(mesh, 1, 0, "scalar_cell_function");
    auto vector_cell_function = make_function(mesh, 3, 0, "vector_cell_function");

    // we can also write out dolfinx meshes directly...
    // in order to properly write parallel output we pass the communicator to the writer
    GridFormat::Writer mesh_writer{GridFormat::vtu, *mesh, mesh->comm()};
    mesh_writer.set_cell_field("rank", [&] (const auto& c) {
        return GridFormat::Parallel::rank(mesh->comm());
    });
    const auto filename = mesh_writer.write("dolfinx_mesh");
    if (GridFormat::Parallel::rank(mesh->comm()) == 0)
        std::cout << "Wrote '" << filename << "'" << std::endl;

    // when running simulations with dolfinx, numerical solutions are typically defined
    // in instances of dolfinx::fem::Function. In order to write out functions (of arbitrary
    // order), we can use a wrapper that is provided in `GridFormat`, and for which all the
    // required traits are specialized. We construct it with one of the nodal spaces, and then
    // we can add data from the other functions to the writer...
    auto lagrange_grid = GridFormat::DolfinX::LagrangePolynomialGrid::from(*scalar_nodal_function.function_space());
    GridFormat::Writer writer{GridFormat::vtu, lagrange_grid, mesh->comm()};
    writer.set_cell_field("rank", [&] (const auto& c) {
        return GridFormat::Parallel::rank(mesh->comm());
    });

    // The wrapper has a convenience function for extracting function values at points/cells.
    // However, you have to specify the rank (0 for scalars; 1 for vectors; 2 for tensors) of
    // the field at compile-time.
    writer.set_point_field("scalar_nodal", [&] (const auto& p) {
        return lagrange_grid.evaluate<0>(scalar_nodal_function, p);
    });

    // But there are predefined convenience functions to add a given function to a writer
    GridFormat::DolfinX::set_point_function(vector_nodal_function, writer);
    GridFormat::DolfinX::set_cell_function(scalar_cell_function, writer);

    // There is also a function that can automatically detect if the given function is nodal
    // or cell-wise and add the field as point/cell field correspondingly:
    GridFormat::DolfinX::set_function(vector_cell_function, writer);

    const auto space_filename = writer.write("dolfinx_spaces");
    if (GridFormat::Parallel::rank(mesh->comm()) == 0)
        std::cout << "Wrote '" << space_filename << "'" << std::endl;

    // The wrapped mesh stores connectivity information of the function, and thus, uses
    // additional memory. For time-dependent simulations, you may want to free that memory
    // during time steps, and update the mesh again before the next write. Note that updating
    // is also necessary in case the mesh changes adaptively. Both updating and clearing is
    // exposed in the API of GridFormat::DolfinX::LagrangeMesh:
    lagrange_grid.clear();
    lagrange_grid.update(*scalar_nodal_function.function_space());
}

int main(int argc, char** argv) {
    PetscInitialize(&argc, &argv, nullptr, nullptr);
    run_fake_simulation();
    PetscFinalize();
    return 0;
}

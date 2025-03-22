// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dolfinx.hpp>

#include <vector>
#include <utility>
#include <optional>
#include <iterator>

#include <dolfinx.h>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"


template<int dim>
auto higher_order_mesh([[maybe_unused]] std::optional<dolfinx::mesh::CellType> ct = {}) {
    if constexpr (dim == 1) {
        return dolfinx::mesh::create_mesh(
            MPI_COMM_WORLD,
            dolfinx::graph::AdjacencyList<std::int64_t>{
                std::vector<std::vector<std::size_t>>{{0, 1, 2}}
            },
            dolfinx::fem::CoordinateElement(dolfinx::mesh::CellType::interval, 2),
            std::vector{
                0., 0., 0.,
                0.5, 0., 0.,
                1., 0., 0.
            },
            {3, 3},
            dolfinx::mesh::GhostMode::none
        );
    } else if constexpr (dim == 2) {
        if (ct.value() == dolfinx::mesh::CellType::triangle)
            return dolfinx::mesh::create_mesh(
                MPI_COMM_WORLD,
                dolfinx::graph::AdjacencyList<std::int64_t>{
                    std::vector<std::vector<std::size_t>>{{0, 1, 2, 3, 4, 5}}
                },
                dolfinx::fem::CoordinateElement(
                    dolfinx::mesh::CellType::triangle,
                    /*order*/2,
                    basix::element::lagrange_variant::equispaced
                ),
                std::vector{
                    0., 0., 0.,
                    1., 0., 0.,
                    0., 1., 0.,
                    0.5, 0.5, 0.,
                    0., 0.5, 0.,
                    0.5, 0., 0.
                },
                {6, 3},
                dolfinx::mesh::GhostMode::none
            );
        else  // quadrilateral
            return dolfinx::mesh::create_mesh(
                MPI_COMM_WORLD,
                dolfinx::graph::AdjacencyList<std::int64_t>{
                    std::vector<std::vector<std::size_t>>{{0, 1, 2, 3, 4, 5, 6, 7, 8}}
                },
                dolfinx::fem::CoordinateElement(dolfinx::mesh::CellType::quadrilateral, 2),
                std::vector{
                    0., 0., 0.,
                    1., 0., 0.,
                    0., 1., 0.,
                    1., 1., 0.,
                    0.5, 0., 0.,
                    0., 0.5, 0.,
                    1., 0.5, 0.,
                    0.5, 1., 0.,
                    0.5, 0.5, 0.
                },
                {9, 3},
                dolfinx::mesh::GhostMode::none
            );
    } else {
        if (ct.value() == dolfinx::mesh::CellType::tetrahedron)
            return dolfinx::mesh::create_mesh(
                MPI_COMM_WORLD,
                dolfinx::graph::AdjacencyList<std::int64_t>{
                    std::vector<std::vector<std::size_t>>{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}}
                },
                dolfinx::fem::CoordinateElement(
                    dolfinx::mesh::CellType::tetrahedron,
                    /*order*/2,
                    basix::element::lagrange_variant::equispaced
                ),
                std::vector{
                    0., 0., 0.,
                    1., 0., 0.,
                    0.5, 1., 0.,
                    0.5, 0.0, 1.0,
                    0.5, 0.5, 0.5,
                    0.75, 0.0, 0.5,
                    0.75, 0.5, 0.0,
                    0.25, 0.0, 0.5,
                    0.25, 0.5, 0.0,
                    0.5, 0.0, 0.0
                },
                {10, 3},
                dolfinx::mesh::GhostMode::none
            );
        else {  // hexahedron
            std::vector<std::size_t> corners;
            std::ranges::copy(std::views::iota(0, 27), std::back_inserter(corners));
            return dolfinx::mesh::create_mesh(
                MPI_COMM_WORLD,
                dolfinx::graph::AdjacencyList<std::int64_t>{
                    std::vector<std::vector<std::size_t>>{corners}
                },
                dolfinx::fem::CoordinateElement(dolfinx::mesh::CellType::hexahedron, 2),
                std::vector{
                    0., 0., 0.,
                    1., 0., 0.,
                    0., 1., 0.,
                    1., 1., 0.,

                    0., 0., 1.,
                    1., 0., 1.,
                    0., 1., 1.,
                    1., 1., 1.,

                    0.5, 0.0, 0.0, // 8
                    0.0, 0.5, 0.0, // 9
                    0.0, 0.0, 0.5, // 10

                    1.0, 0.5, 0.0, // 11
                    1.0, 0.0, 0.5, // 12
                    0.5, 1.0, 0.0, // 13

                    0.0, 1.0, 0.5, // 14
                    1.0, 1.0, 0.5, // 15
                    0.5, 0.0, 1.0, // 16

                    0.0, 0.5, 1.0, // 17
                    1.0, 0.5, 1.0, // 18
                    0.5, 1.0, 1.0, // 19

                    0.5, 0.5, 0.0, // 20
                    0.5, 0.0, 0.5, // 21
                    0.0, 0.5, 0.5, // 22

                    1.0, 0.5, 0.5, // 23
                    0.5, 1.0, 0.5, // 24
                    0.5, 0.5, 1.0, // 25

                    0.5, 0.5, 0.5  // 26
                },
                {27, 3},
                dolfinx::mesh::GhostMode::none
            );
        }
    }
}

bool is_sequential() {
    return GridFormat::Parallel::size(MPI_COMM_WORLD) == 1;
}

std::string get_filename(dolfinx::mesh::CellType ct, const std::string& suffix = "") {
    return "dolfinx_vtu" + (suffix.empty() ? "" : "_" + suffix) + "_"
        + dolfinx::mesh::to_string(ct) + "_nranks_"
        + std::to_string(GridFormat::Parallel::size(MPI_COMM_WORLD)) + "_"
        + std::to_string(dolfinx::mesh::cell_dim(ct)) + "d_in_3d";
}

template<typename Writer>
void write_with(Writer writer, const std::string& filename) {
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& p) {
        return GridFormat::Test::test_function<double>(GridFormat::coordinates(writer.grid(), p));
    });
    writer.set_cell_field("cfunc", [&] (const auto& p) {
        std::array<double, 3> center;
        std::ranges::fill(center, 0.0);
        int corner_count = 0;
        for (const auto& p : GridFormat::points(writer.grid(), p)) {
            const auto pos = GridFormat::coordinates(writer.grid(), p);
            for (int i = 0; i < 3; ++i)
                center[i] += pos[i];
            corner_count++;
        }
        std::ranges::for_each(center, [&] (auto& v) { v /= corner_count; });
        return GridFormat::Test::test_function<double>(center);
    });
    const auto written_filename = writer.write(filename);
    if (GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0)
        std::cout << "Wrote '" << written_filename << "'" << std::endl;

    // Run a bunch of unit tests with the given grid
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    const auto& grid = writer.grid();
    using Grid = std::decay_t<decltype(grid)>;

    "number_of_cells"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::cells(grid))),
            static_cast<std::size_t>(GridFormat::Traits::NumberOfCells<Grid>::get(grid))
        ));
    };
    "number_of_vertices"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid))),
            static_cast<std::size_t>(GridFormat::Traits::NumberOfPoints<Grid>::get(grid))
        ));
    };
    "number_of_cell_points"_test = [&] () {
        for (const auto& c : GridFormat::cells(grid))
            expect(eq(
                static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid, c))),
                static_cast<std::size_t>(GridFormat::Traits::NumberOfCellPoints<Grid, std::decay_t<decltype(c)>>::get(grid, c))
            ));
    };
}

void write(const dolfinx::mesh::Mesh& mesh, std::string suffix = "") {
    const auto added_suffix = suffix.empty() ? "" : "_" + suffix;
    write_with(GridFormat::PVTUWriter{mesh, MPI_COMM_WORLD}, get_filename(mesh.topology().cell_type(), added_suffix));
    if (is_sequential())
        write_with(GridFormat::VTUWriter{mesh}, get_filename(mesh.topology().cell_type(), "sequential" + added_suffix));
}

template<int dim>
void write() {
    if constexpr (dim == 1) {
        write(dolfinx::mesh::create_interval(MPI_COMM_WORLD, 5, {0., 1.0}));
        write(dolfinx::mesh::create_interval(
            MPI_COMM_WORLD, 5, {0., 1.0},
            dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_facet)
        ));
        write(dolfinx::mesh::create_interval(
            MPI_COMM_WORLD, 5, {0., 1.0},
            dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_vertex)
        ));

        if (is_sequential()) {
            const auto& mesh = higher_order_mesh<dim>();
            write_with(
                GridFormat::VTUWriter{mesh},
                get_filename(dolfinx::mesh::CellType::interval, "higher_order")
            );
        }
    }
    if constexpr (dim == 2) {
        auto min = GridFormat::Ranges::filled_array<dim>(0.0);
        auto max = GridFormat::Ranges::filled_array<dim>(1.0);
        for (auto ct : std::vector{dolfinx::mesh::CellType::triangle, dolfinx::mesh::CellType::quadrilateral}) {
            write(dolfinx::mesh::create_rectangle(MPI_COMM_WORLD, {min, max}, {4, 4}, ct));
            write(dolfinx::mesh::create_rectangle(
                MPI_COMM_WORLD, {min, max}, {4, 4}, ct,
                dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_facet)
            ), "shared_facet");
            write(dolfinx::mesh::create_rectangle(
                MPI_COMM_WORLD, {min, max}, {4, 4}, ct,
                dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_vertex)
            ), "shared_vertex");

            if (is_sequential()) {
                const auto& mesh = higher_order_mesh<dim>(ct);
                write_with(
                    GridFormat::VTUWriter{mesh},
                    get_filename(ct, "higher_order")
                );
            }
        }
    }
    if constexpr (dim == 3) {
        auto min = GridFormat::Ranges::filled_array<dim>(0.0);
        auto max = GridFormat::Ranges::filled_array<dim>(1.0);
        for (auto ct : std::vector{dolfinx::mesh::CellType::tetrahedron, dolfinx::mesh::CellType::hexahedron}) {
            write(dolfinx::mesh::create_box(MPI_COMM_WORLD, {min, max}, {4, 4, 4}, ct));
            write(dolfinx::mesh::create_box(
                MPI_COMM_WORLD, {min, max}, {4, 4, 4}, ct,
                dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_facet)
            ), "shared_facet");
            write(dolfinx::mesh::create_box(
                MPI_COMM_WORLD, {min, max}, {4, 4, 4}, ct,
                dolfinx::mesh::create_cell_partitioner(dolfinx::mesh::GhostMode::shared_vertex)
            ), "shared_vertex");

            if (is_sequential()) {
                const auto mesh = higher_order_mesh<dim>(ct);
                write_with(
                    GridFormat::VTUWriter{mesh}.with_encoding(GridFormat::Encoding::ascii),
                    get_filename(ct, "higher_order")
                );
            }
        }
    }
}

auto make_hex_function_space(std::shared_ptr<dolfinx::mesh::Mesh> mesh, int order, int block_size) {
    return std::make_shared<const dolfinx::fem::FunctionSpace>(dolfinx::fem::create_functionspace(
        mesh,
        basix::create_element(
            basix::element::family::P,
            dolfinx::mesh::cell_type_to_basix_type(dolfinx::mesh::CellType::hexahedron),
            order,
            basix::element::lagrange_variant::unset,
            basix::element::dpc_variant::unset,
            (order == 0 ? true : false) // discontinuous?
        ),
        block_size
    ));
}

auto make_function(std::shared_ptr<const dolfinx::fem::FunctionSpace> space) {
    dolfinx::fem::Function<double> function{space};
    function.interpolate([&] (auto x) {
        const auto n_points = x.extent(1);
        const auto block_size = space->element()->block_size();
        std::vector<double> data(n_points*block_size, 0.0);
        for (int c = 0; c < block_size; ++c) {
            for (unsigned int i = 0; i < n_points; ++i)
                data[n_points*c + i] = GridFormat::Test::test_function<double>(
                    std::array{x(0, i), x(1, i), x(2, i)}
                );
        }
        const auto shape = block_size > 1 ? std::vector<std::size_t>{static_cast<std::size_t>(block_size), n_points}
                                          : std::vector{n_points};
        return std::make_pair(data, shape);
    });
    return function;
}

int main(int argc, char** argv) {
    PetscInitialize(&argc, &argv, nullptr, nullptr);
    write<1>();
    write<2>();
    write<3>();

    {
        // test writing from a higher-order functions
        // we need the braces so that everything goes out of scope before we call finalize
        const auto mesh = std::make_shared<dolfinx::mesh::Mesh>(dolfinx::mesh::create_box(
            MPI_COMM_WORLD,
            {
                std::array{0.0, 0.0, 0.0},
                std::array{1.0, 1.0, 1.0}
            },
            {4, 4, 4},
            dolfinx::mesh::CellType::hexahedron
        ));
        auto scalar_nodal_function = make_function(make_hex_function_space(mesh, 2, 1));
        auto vector_nodal_function = make_function(make_hex_function_space(mesh, 2, 3));
        auto scalar_cell_function = make_function(make_hex_function_space(mesh, 0, 1));
        auto vector_cell_function = make_function(make_hex_function_space(mesh, 0, 3));

        auto lagrange_grid = GridFormat::DolfinX::LagrangePolynomialGrid::from(*scalar_nodal_function.function_space());
        GridFormat::PVTUWriter writer{lagrange_grid, MPI_COMM_WORLD};
        GridFormat::Test::add_meta_data(writer);
        writer.set_point_field("pfunc", [&] (const auto& p) { return lagrange_grid.evaluate(scalar_nodal_function, p); });
        writer.set_point_field("pfunc_vec", [&] (const auto& p) { return lagrange_grid.evaluate<1>(vector_nodal_function, p); });
        writer.set_cell_field("cfunc", [&] (const auto& p) { return lagrange_grid.evaluate(scalar_cell_function, p); });
        writer.set_cell_field("cfunc_vec", [&] (const auto& p) { return lagrange_grid.evaluate<1>(vector_cell_function, p); });
        GridFormat::DolfinX::set_point_function(scalar_nodal_function, writer, "pfunc_via_freefunction");
        GridFormat::DolfinX::set_point_function(vector_nodal_function, writer, "pfunc_vec_via_freefunction");
        GridFormat::DolfinX::set_cell_function(scalar_cell_function, writer, "cfunc_via_freefunction");
        GridFormat::DolfinX::set_cell_function(vector_cell_function, writer, "cfunc_vec_via_freefunction");
        GridFormat::DolfinX::set_function(scalar_nodal_function, writer, "pfunc_via_auto_freefunction");
        GridFormat::DolfinX::set_function(vector_nodal_function, writer, "pfunc_vec_via_auto_freefunction");
        GridFormat::DolfinX::set_function(scalar_cell_function, writer, "cfunc_via_auto_freefunction");
        GridFormat::DolfinX::set_function(vector_cell_function, writer, "cfunc_vec_via_auto_freefunction");

        const auto prec = GridFormat::float32;
        GridFormat::DolfinX::set_point_function(scalar_nodal_function, writer, "pfunc_float32_via_freefunction", prec);
        GridFormat::DolfinX::set_cell_function(scalar_cell_function, writer, "cfunc_float32_via_freefunction", prec);
        GridFormat::DolfinX::set_function(scalar_cell_function, writer, "cfunc_float32_via_auto_freefunction", prec);

        const auto filename = writer.write(get_filename(mesh->topology().cell_type(), "from_space"));
        if (GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0)
            std::cout << "Wrote '" << filename << "'" << std::endl;

        // A bunch of unit tests ...
        using GridFormat::Testing::operator""_test;
        using GridFormat::Testing::expect;
        using GridFormat::Testing::throws;

        // test name deduction from field
        vector_nodal_function.name = "from_point_vector_function_name";
        scalar_nodal_function.name = "from_point_function_name";
        scalar_cell_function.name = "from_cell_function_name";
        "field_setter_name_from_function"_test = [&] () {
            GridFormat::DolfinX::set_point_function(scalar_nodal_function, writer);
            GridFormat::DolfinX::set_cell_function(scalar_cell_function, writer);
            GridFormat::DolfinX::set_function(vector_nodal_function, writer);
            expect(std::ranges::any_of(point_fields(writer), [] (const auto& pair) {
                return pair.first == "from_point_function_name";
            }));
            expect(std::ranges::any_of(point_fields(writer), [] (const auto& pair) {
                return pair.first == "from_point_vector_function_name";
            }));
            expect(std::ranges::any_of(cell_fields(writer), [] (const auto& pair) {
                return pair.first == "from_cell_function_name";
            }));
        };

        "lagrange_grid_clear"_test = [&] () {
            lagrange_grid.clear();
            expect(throws<GridFormat::InvalidState>([&] () { lagrange_grid.cells(); }));
            expect(throws<GridFormat::InvalidState>([&] () { lagrange_grid.points(); }));
        };

        "lagrange_grid_update"_test = [&] () {
            lagrange_grid.update(*scalar_nodal_function.function_space());
            lagrange_grid.cells();
            lagrange_grid.points();
        };

        const auto different_mesh = std::make_shared<dolfinx::mesh::Mesh>(dolfinx::mesh::create_box(
            MPI_COMM_WORLD,
            {
                std::array{0.0, 0.0, 0.0},
                std::array{1.0, 1.0, 1.0}
            },
            {5, 4, 4},
            dolfinx::mesh::CellType::hexahedron
        ));
        const auto nodal_function_different_mesh = make_function(make_hex_function_space(different_mesh, 2, 1));
        const auto cell_function_different_mesh = make_function(make_hex_function_space(different_mesh, 0, 1));

        "field_setter_throws_with_different_mesh"_test = [&] () {
            expect(throws<GridFormat::ValueError>([&] () {
                GridFormat::DolfinX::set_point_function(nodal_function_different_mesh, writer);
            }));
            expect(throws<GridFormat::ValueError>([&] () {
                GridFormat::DolfinX::set_cell_function(cell_function_different_mesh, writer);
            }));
            expect(throws<GridFormat::ValueError>([&] () {
                GridFormat::DolfinX::set_function(cell_function_different_mesh, writer);
            }));
        };

        "field_setter_throws_for_nonmatching_space"_test = [&] () {
            expect(throws<GridFormat::ValueError>([&] () {
                GridFormat::DolfinX::set_point_function(scalar_cell_function, writer);
            }));
            expect(throws<GridFormat::ValueError>([&] () {
                GridFormat::DolfinX::set_cell_function(scalar_nodal_function, writer);
            }));
        };

        "dolfinx_lagrange_grid_fails_to_construct_from_p0_space"_test = [&] () {
            expect(throws([&] () { GridFormat::DolfinX::LagrangePolynomialGrid::from(*scalar_cell_function.function_space()); }));
        };
    }

    PetscFinalize();
    return 0;
}

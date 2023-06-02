// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <utility>
#include <optional>
#include <iterator>

#include <dolfinx.h>

#include <gridformat/grid/adapters/dolfinx.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include "../make_test_data.hpp"


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
        + dolfinx::mesh::to_string(ct) + "_"
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

        if (is_sequential())
            write_with(
                GridFormat::VTUWriter{higher_order_mesh<dim>()},
                get_filename(dolfinx::mesh::CellType::interval, "higher_order")
            );
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

            if (is_sequential())
                write_with(
                    GridFormat::VTUWriter{higher_order_mesh<dim>(ct)},
                    get_filename(ct, "higher_order")
                );
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
                write_with(
                    GridFormat::VTUWriter{higher_order_mesh<dim>(ct)}.with_encoding(GridFormat::Encoding::ascii),
                    get_filename(ct, "higher_order")
                );
                dolfinx::io::VTKFile file{MPI_COMM_WORLD, std::string{"mesh_"} + dolfinx::mesh::to_string(ct) + ".pvd", "w"};
                file.write(higher_order_mesh<dim>(ct), 0.0);
            }
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    dolfinx::init_logging(argc, argv);

    write<1>();
    write<2>();
    write<3>();

    MPI_Finalize();

    return 0;
}

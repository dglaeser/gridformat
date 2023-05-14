// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <ranges>
#include <cmath>

#include <gridformat/common/logging.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/image_grid.hpp>

// In the GitHub action runner we run into a compiler warning when
// using release flags. Locally, this could not be reproduced. For
// now, we simply ignore those warnings here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#include <gridformat/vtk/vti_writer.hpp>
#pragma GCC diagnostic pop
#include <gridformat/vtk/vtr_writer.hpp>
#include <gridformat/vtk/vts_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>

#include "../testing.hpp"
#include "../make_test_data.hpp"


template<typename Writer, std::size_t dim, typename CT>
void write_test_file(Writer&& w,
                     const GridFormat::ImageGrid<dim, CT>& grid,
                     const std::string& prefix) {
    GridFormat::Test::add_meta_data(w);
    w.set_point_field("pfunc", [&] (const auto& p) {
        return GridFormat::Test::test_function<double>(grid.position(p));
    });
    w.set_cell_field("cfunc", [&] (const auto& c) {
        return GridFormat::Test::test_function<double>(grid.center(c));
    });
    std::cout << "Wrote '" << GridFormat::as_highlight(
        w.write(prefix + "_image_grid_test_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d")
    ) << "'" << std::endl;
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    GridFormat::ImageGrid grid{
        std::array{1.0, 1.0},
        std::array{10, 12}
    };

    static_assert(GridFormat::Concepts::ImageGrid<GridFormat::ImageGrid<2, double>>);
    static_assert(GridFormat::Concepts::ImageGrid<GridFormat::ImageGrid<3, double>>);

    static_assert(GridFormat::Concepts::RectilinearGrid<GridFormat::ImageGrid<2, double>>);
    static_assert(GridFormat::Concepts::RectilinearGrid<GridFormat::ImageGrid<3, double>>);

    static_assert(GridFormat::Concepts::StructuredGrid<GridFormat::ImageGrid<2, double>>);
    static_assert(GridFormat::Concepts::StructuredGrid<GridFormat::ImageGrid<3, double>>);

    static_assert(GridFormat::Concepts::UnstructuredGrid<GridFormat::ImageGrid<2, double>>);
    static_assert(GridFormat::Concepts::UnstructuredGrid<GridFormat::ImageGrid<3, double>>);

    "structured_grid_number_of_cells"_test = [&] () {
        expect(eq(grid.number_of_cells(), 120ul));
    };

    "structured_grid_number_of_cells_per_dir"_test = [&] () {
        expect(eq(grid.number_of_cells(0), 10ul));
        expect(eq(grid.number_of_cells(1), 12ul));
    };

    "structured_grid_number_of_points"_test = [&] () {
        expect(eq(grid.number_of_points(), 143ul));
    };

    "structured_grid_number_of_points_per_dir"_test = [&] () {
        expect(eq(grid.number_of_points(0), 11ul));
        expect(eq(grid.number_of_points(1), 13ul));
    };

    "structured_grid_cells_iterator_size"_test = [&] () {
        expect(eq(GridFormat::Ranges::size(cells(grid)), 120ul));
    };

    "structured_grid_points_iterator_size"_test = [&] () {
        expect(eq(GridFormat::Ranges::size(points(grid)), 143ul));
    };

    "structured_grid_spacing"_test = [&] () {
        expect(std::abs(grid.spacing()[0] - 0.1) < 1e-6);
        expect(std::abs(grid.spacing()[1] - 1.0/12) < 1e-6);
    };

    "structured_grid_extents"_test = [&] () {
        expect(eq(grid.extents()[0], 10ul));
        expect(eq(grid.extents()[1], 12ul));
    };

    write_test_file(GridFormat::VTIWriter{grid}, grid, "vti");
    write_test_file(GridFormat::VTRWriter{grid}, grid, "vtr");
    write_test_file(GridFormat::VTSWriter{grid}, grid, "vts");
    write_test_file(GridFormat::VTPWriter{grid}, grid, "vtp");
    write_test_file(GridFormat::VTUWriter{grid}, grid, "vtu");

    GridFormat::ImageGrid grid_3d{
        std::array{1.0, 1.2, 1.4},
        std::array{6, 8, 10}
    };

    write_test_file(GridFormat::VTIWriter{grid_3d}, grid_3d, "vti");
    write_test_file(GridFormat::VTRWriter{grid_3d}, grid_3d, "vtr");
    write_test_file(GridFormat::VTSWriter{grid_3d}, grid_3d, "vts");
    write_test_file(GridFormat::VTPWriter{grid_3d}, grid_3d, "vtp");
    write_test_file(GridFormat::VTUWriter{grid_3d}, grid_3d, "vtu");

    return 0;
}

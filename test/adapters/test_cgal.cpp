// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_3.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Regular_triangulation_2.h>
#include <CGAL/Regular_triangulation_3.h>
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/barycenter.h>
#pragma GCC diagnostic pop

#include <gridformat/traits/cgal.hpp>

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/discontinuous.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"


// register the missing trait - cgal does not have a standard way of retrieving vertex indices
// in this test, we use the widely-used approach of attaching an info object to the vertices
namespace GridFormat::Traits {

template<GridFormat::Concepts::CGALGrid T>
struct PointId<T, typename T::Vertex> {
    static std::size_t get(const T&, const typename T::Vertex& vertex) {
        return vertex.info();
    }
};

}  // namespace GridFormat::Traits


template<typename Kernel, typename Base = CGAL::Triangulation_vertex_base_2<Kernel>>
using VertexWithIndex2D = CGAL::Triangulation_vertex_base_with_info_2<unsigned, Kernel, Base>;
template<typename Kernel, typename Base = CGAL::Triangulation_vertex_base_3<Kernel>>
using VertexWithIndex3D = CGAL::Triangulation_vertex_base_with_info_3<unsigned, Kernel, Base>;

template<typename Kernel,
         typename Face = CGAL::Triangulation_face_base_2<Kernel>,
         typename BaseVertex = CGAL::Triangulation_vertex_base_2<Kernel>>
using TDS2D = CGAL::Triangulation_data_structure_2<VertexWithIndex2D<Kernel, BaseVertex>, Face>;
template<typename Kernel,
         typename Cell = CGAL::Triangulation_cell_base_3<Kernel>,
         typename BaseVertex = CGAL::Triangulation_vertex_base_3<Kernel>>
using TDS3D = CGAL::Triangulation_data_structure_3<VertexWithIndex3D<Kernel, BaseVertex>, Cell>;

template<typename K>
using RegularTriangulation2D = CGAL::Regular_triangulation_2<K, TDS2D<K,
    CGAL::Regular_triangulation_face_base_2<K>,
    CGAL::Regular_triangulation_vertex_base_2<K>
>>;

template<typename K>
using RegularTriangulation3D = CGAL::Regular_triangulation_3<K, TDS3D<K,
    CGAL::Regular_triangulation_cell_base_3<K>,
    CGAL::Regular_triangulation_vertex_base_3<K>
>>;

template<typename K>
using ConstrainedTriangulation2D = CGAL::Constrained_triangulation_2<K, TDS2D<K,
    CGAL::Constrained_triangulation_face_base_2<K>
>>;

template<typename K>
using ConstrainedDelaunayTriangulation2D = CGAL::Constrained_Delaunay_triangulation_2<K, TDS2D<K,
    CGAL::Constrained_triangulation_face_base_2<K>
>>;


// helper function to compute the center of a grid cell
template<typename FT, std::ranges::range R>
    requires(!GridFormat::Concepts::CGALPointWrapper<std::ranges::range_value_t<R>>)
auto cell_center(R&& points) {
    const auto corners = points | std::views::transform([] (const auto& p) {
        return std::make_pair(p, FT{1});
    });
    return CGAL::barycenter(std::ranges::begin(corners), std::ranges::end(corners));
}

template<typename FT, std::ranges::range R>
    requires(GridFormat::Concepts::CGALPointWrapper<std::ranges::range_value_t<R>>)
auto cell_center(R&& points) {
    return cell_center<FT>(points | std::views::transform([&] (const auto& wp) { return wp.point(); }));
}

template<typename Kernel, typename CGALCell>
auto cell_center(const CGALCell& cell, int num_corners) {
    return cell_center<typename Kernel::FT>(std::views::iota(0, num_corners) | std::views::transform([&] (int i) {
        return cell.vertex(i)->point();
    }));
}

template<typename T>
void insert_points_2d(T& triangulation) {
    triangulation.insert(typename T::Point{0., 0.});
    triangulation.insert(typename T::Point{1., 0.});
    triangulation.insert(typename T::Point{1., 1.});
    triangulation.insert(typename T::Point{0., 1.});

    unsigned int i = 0;
    for (auto vertex_handle : triangulation.finite_vertex_handles())
        vertex_handle->info() = i++;
}

template<typename T>
void insert_points_3d(T& triangulation) {
    triangulation.insert(typename T::Point{0., 0., 0.});
    triangulation.insert(typename T::Point{1., 0., 0.});
    triangulation.insert(typename T::Point{0., 1., 0.});
    triangulation.insert(typename T::Point{1., 1., 0.});

    triangulation.insert(typename T::Point{0., 0., 1.});
    triangulation.insert(typename T::Point{1., 0., 1.});
    triangulation.insert(typename T::Point{0., 1., 1.});
    triangulation.insert(typename T::Point{1., 1., 1.});

    unsigned int i = 0;
    for (auto vertex_handle : triangulation.finite_vertex_handles())
        vertex_handle->info() = i++;
}

template<template<typename> typename Writer = GridFormat::VTUWriter,
         GridFormat::Concepts::CGALGrid Grid>
void write(Grid grid, std::string prefix_addition = "") {
    static constexpr int dim = GridFormat::CGAL::dimension<Grid>;
    if constexpr (dim == 2)
        insert_points_2d(grid);
    else
        insert_points_3d(grid);

    Writer writer{grid};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(
            GridFormat::CGAL::to_double_array(vertex->point())
        );
    });
    writer.set_cell_field("cfunc", [&] (const auto& cell) {
        const int num_corners = dim == 2 ? 3 : 4;
        return GridFormat::Test::test_function<double>(
            GridFormat::CGAL::to_double_array(cell_center<typename Grid::Geom_traits>(cell, num_corners))
        );
    });

    prefix_addition = prefix_addition.empty() ? "" : "_" + prefix_addition;
    const auto filename = "cgal_vtu" + prefix_addition + "_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d";
    std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(filename)) << "'" << std::endl;

    // write a discontinuous file to make sure the discontinuous grid wrapper works with CGAL grids
    GridFormat::DiscontinuousGrid discontinuous{grid};
    Writer discontinuous_writer{discontinuous};
    GridFormat::Test::add_meta_data(discontinuous_writer);
    GridFormat::Test::add_discontinuous_point_field(discontinuous_writer);
    discontinuous_writer.write(filename + "_discontinuous");

    // Run a bunch of unit tests with the given grid
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "number_of_cells"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::cells(grid))),
            GridFormat::Traits::NumberOfCells<Grid>::get(grid)
        ));
    };
    "number_of_vertices"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid))),
            GridFormat::Traits::NumberOfPoints<Grid>::get(grid)
        ));
    };
    "number_of_cell_points"_test = [&] () {
        for (const auto& c : GridFormat::cells(grid))
            expect(eq(
                static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid, c))),
                GridFormat::Traits::NumberOfCellPoints<Grid, std::decay_t<decltype(c)>>::get(grid, c)
            ));
    };
}

int main() {
    using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
    using ExactKernel = CGAL::Exact_predicates_exact_constructions_kernel;

    write(CGAL::Triangulation_2<Kernel, TDS2D<Kernel>>{});
    write(CGAL::Triangulation_2<ExactKernel, TDS2D<ExactKernel>>{}, "exact");
    write(CGAL::Delaunay_triangulation_2<Kernel, TDS2D<Kernel>>{}, "delaunay");
    write(CGAL::Delaunay_triangulation_2<ExactKernel, TDS2D<ExactKernel>>{}, "delaunay_exact");

    write<GridFormat::VTPWriter>(CGAL::Delaunay_triangulation_2<ExactKernel, TDS2D<ExactKernel>>{}, "delaunay_exact_as_poly");

    write(RegularTriangulation2D<Kernel>{}, "regular");
    write(RegularTriangulation2D<ExactKernel>{}, "regular_exact");

    write(ConstrainedTriangulation2D<Kernel>{}, "constrained");
    write(ConstrainedTriangulation2D<ExactKernel>{}, "constrained_exact");

    write(ConstrainedDelaunayTriangulation2D<Kernel>{}, "constrained_delaunay");
    write(ConstrainedDelaunayTriangulation2D<ExactKernel>{}, "constrained_delaunay_exact");

    // Three-dimensional grids
    write(CGAL::Triangulation_3<Kernel, TDS3D<Kernel>>{});
    write(CGAL::Triangulation_3<ExactKernel, TDS3D<ExactKernel>>{}, "exact");
    write(CGAL::Delaunay_triangulation_3<Kernel, TDS3D<Kernel>>{}, "delaunay");
    write(CGAL::Delaunay_triangulation_3<ExactKernel, TDS3D<ExactKernel>>{}, "delaunay_exact");
    write(RegularTriangulation3D<Kernel>{}, "regular");
    write(RegularTriangulation3D<ExactKernel>{}, "regular_exact");

    return 0;
}

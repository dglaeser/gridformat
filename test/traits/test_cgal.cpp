// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/cgal.hpp>

#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
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

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/discontinuous.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"


// we wrap cgal grids once more, to test if the discontinuous grid
// wrapper also works if we capture a cell by reference in the CellPoints trait.
// This reference would be dangling because cgal grids yield temporaries as cells
// (namely handles), but we built in extra logic in the discontinuous grid wrapper
// to support this case.
template<typename CGALGrid>
class CGALGridTestWrapper {
 public:
    CGALGridTestWrapper(const CGALGrid& grid) : _grid{grid} {}
    operator const CGALGrid&() const { return _grid; }
 private:
    const CGALGrid& _grid;
};

namespace GridFormat::Traits {
template<typename G> struct Cells<CGALGridTestWrapper<G>> : public Cells<G> {};
template<typename G> struct Points<CGALGridTestWrapper<G>> : public Points<G> {};

template<typename G> struct NumberOfPoints<CGALGridTestWrapper<G>> : public NumberOfPoints<G> {};
template<typename G> struct NumberOfCells<CGALGridTestWrapper<G>> : public NumberOfCells<G> {};

template<typename G> struct PointCoordinates<CGALGridTestWrapper<G>, Point<G>> : public PointCoordinates<G, Point<G>> {};
template<typename G> struct PointId<CGALGridTestWrapper<G>, Point<G>> : public PointId<G, Point<G>> {};
template<typename G> struct CellPoints<CGALGridTestWrapper<G>, Cell<G>> : public CellPoints<G, Cell<G>> {};

template<typename G> struct CellType<CGALGridTestWrapper<G>, Cell<G>> : public CellType<G, Cell<G>> {};
template<typename G> struct NumberOfCellPoints<CGALGridTestWrapper<G>, Cell<G>> {
    static std::ranges::range auto get(const CGALGridTestWrapper<G>& grid, const Cell<G>& cell) {
        return std::views::iota(0, NumberOfCellPoints<G, Cell<G>>::get(grid.host_grid(), cell.host_cell()))
            | std::views::transform([&] (std::integral auto i) {
                return cell->vertex(i);
            });
    }
};
}  // namespace GridFormat::Traits


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
}

void print_write_message(const std::string& filename) {
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
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
            GridFormat::CGAL::to_double_array(cell_center<typename Grid::Geom_traits>(*cell, num_corners))
        );
    });

    prefix_addition = prefix_addition.empty() ? "" : "_" + prefix_addition;
    const auto filename = "cgal_vtu" + prefix_addition + "_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d";
    print_write_message(writer.write(filename));

    // write a discontinuous file to make sure the discontinuous grid wrapper works with CGAL grids
    {
        GridFormat::DiscontinuousGrid discontinuous{grid};
        Writer discontinuous_writer{discontinuous};
        GridFormat::Test::add_meta_data(discontinuous_writer);
        GridFormat::Test::add_discontinuous_point_field(discontinuous_writer);
        print_write_message(discontinuous_writer.write(filename + "_discontinuous"));
    }

    {  // .. and wrap it (see comments at the beginning of this file)
        CGALGridTestWrapper wrapped{grid};
        GridFormat::DiscontinuousGrid discontinuous{wrapped};
        Writer discontinuous_writer{discontinuous};
        GridFormat::Test::add_meta_data(discontinuous_writer);
        GridFormat::Test::add_discontinuous_point_field(discontinuous_writer);
        print_write_message(discontinuous_writer.write(filename + "_discontinuous_wrapped"));
    }

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

    write(CGAL::Triangulation_2<Kernel>{});
    write(CGAL::Triangulation_2<ExactKernel>{}, "exact");
    write(CGAL::Delaunay_triangulation_2<Kernel>{}, "delaunay");
    write(CGAL::Delaunay_triangulation_2<ExactKernel>{}, "delaunay_exact");

    write<GridFormat::VTPWriter>(CGAL::Delaunay_triangulation_2<ExactKernel>{}, "delaunay_exact_as_poly");

    write(CGAL::Regular_triangulation_2<Kernel>{}, "regular");
    write(CGAL::Regular_triangulation_2<ExactKernel>{}, "regular_exact");

    write(CGAL::Constrained_triangulation_2<Kernel>{}, "constrained");
    write(CGAL::Constrained_triangulation_2<ExactKernel>{}, "constrained_exact");

    write(CGAL::Constrained_Delaunay_triangulation_2<Kernel>{}, "constrained_delaunay");
    write(CGAL::Constrained_Delaunay_triangulation_2<ExactKernel>{}, "constrained_delaunay_exact");

    // Three-dimensional grids
    write(CGAL::Triangulation_3<Kernel>{});
    write(CGAL::Triangulation_3<ExactKernel>{}, "exact");
    write(CGAL::Delaunay_triangulation_3<Kernel>{}, "delaunay");
    write(CGAL::Delaunay_triangulation_3<ExactKernel>{}, "delaunay_exact");
    write(CGAL::Regular_triangulation_3<Kernel>{}, "regular");
    write(CGAL::Regular_triangulation_3<ExactKernel>{}, "regular_exact");

    return 0;
}

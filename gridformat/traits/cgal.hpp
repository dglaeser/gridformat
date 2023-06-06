// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup PredefinedTraits
 * \brief Traits specializations for cgal triangulations in
 *        <a href="https://doc.cgal.org/latest/Triangulation_2/index.html">2D</a>
 *        <a href="https://doc.cgal.org/latest/Triangulation_3/index.html">3D</a>.
 */
#ifndef GRIDFORMAT_TRAITS_CGAL_HPP_
#define GRIDFORMAT_TRAITS_CGAL_HPP_

#include <cassert>
#include <ranges>
#include <vector>
#include <array>
#include <concepts>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/cell_type.hpp>

// Forward declaration of the triangulation classes.
// Users are expected to include the actual headers.
// Note: we also need CGAL::to_double(), and therefore,
//       users must include the cgal headers before this one!
// Note: there is no general way of retrieving the vertex indices
//       of a cgal triangulation. The typical way is to attach an info
//       object containing the index to the data structure used for vertices.
//       Users must provide the PointID trait for their application...
namespace CGAL {

template<typename T, typename TDS> class Triangulation_2;
template<typename T, typename TDS, typename SLDS> class Triangulation_3;
template<typename T> class Point_2;
template<typename T> class Point_3;

}  // namespace CGAL


namespace GridFormat {

namespace Concepts {

#ifndef DOXYGEN
namespace CGALDetail {

    // helper function to deduce the actually set LockDataStructure
    // the thing is that Triangulation_3 exports a LockDataStructure type that
    // may be different from the one originally set by the user.
    template<typename K, typename TDS, typename LDS>
    LDS deduce_lds(const CGAL::Triangulation_3<K, TDS, LDS>&) { return {}; }

    template<typename T> struct IsPoint : public std::false_type {};
    template<typename K> struct IsPoint<CGAL::Point_2<K>> : public std::true_type {};
    template<typename K> struct IsPoint<CGAL::Point_3<K>> : public std::true_type {};
    template<typename T> concept Point = IsPoint<std::remove_cvref_t<T>>::value;

}  // CGALDetail
#endif  // DOXYGEN

template<typename T>
concept CGALGrid2D = requires {
    typename T::Geom_traits;
    typename T::Triangulation_data_structure;
    requires std::derived_from<T, CGAL::Triangulation_2<
        typename T::Geom_traits,
        typename T::Triangulation_data_structure
    >>;
};

template<typename T>
concept CGALGrid3D = requires(const T& grid) {
    typename T::Geom_traits;
    typename T::Triangulation_data_structure;
    typename T::Lock_data_structure;
    requires std::derived_from<T, CGAL::Triangulation_3<
        typename T::Geom_traits,
        typename T::Triangulation_data_structure,
        decltype(CGALDetail::deduce_lds(grid))
    >>;
};

template<typename T>
concept CGALGrid = CGALGrid2D<T> or CGALGrid3D<T>;

// Some point types in CGAL (as weighted_point) wrap a CGAL::Point
template<typename T>
concept CGALPointWrapper = requires(const T& wrapper) {
    { wrapper.point() } -> CGALDetail::Point;
};

}  // namespace Concepts

namespace CGAL {

template<typename T> struct CellType;
template<Concepts::CGALGrid2D T> struct CellType<T> : public std::type_identity<typename T::Face> {};
template<Concepts::CGALGrid3D T> struct CellType<T> : public std::type_identity<typename T::Cell> {};
template<Concepts::CGALGrid T> using Cell = typename CellType<T>::type;

template<Concepts::CGALGrid T>
inline constexpr int dimension = Concepts::CGALGrid2D<T> ? 2 : 3;

template<typename K>
std::array<double, 2> to_double_array(const ::CGAL::Point_2<K>& p) {
    return {::CGAL::to_double(p.x()), ::CGAL::to_double(p.y())};
}

template<typename K>
std::array<double, 3> to_double_array(const ::CGAL::Point_3<K>& p) {
    return {::CGAL::to_double(p.x()), ::CGAL::to_double(p.y()), ::CGAL::to_double(p.z())};
}
template<Concepts::CGALPointWrapper W>
auto to_double_array(const W& wrapper) { return to_double_array(wrapper.point()); }

}  // namespace CGAL

namespace Traits {

template<Concepts::CGALGrid Grid>
struct Cells<Grid> {
    static std::ranges::range auto get(const Grid& grid) {
        if constexpr (Concepts::CGALGrid2D<Grid>)
            return std::ranges::subrange(grid.finite_faces_begin(), grid.finite_faces_end());
        else
            return std::ranges::subrange(grid.finite_cells_begin(), grid.finite_cells_end());
    }
};

template<Concepts::CGALGrid Grid>
struct Points<Grid> {
    static std::ranges::range auto get(const Grid& grid) {
        return std::ranges::subrange(grid.finite_vertices_begin(), grid.finite_vertices_end());
    }
};

template<Concepts::CGALGrid Grid>
struct CellPoints<Grid, GridFormat::CGAL::Cell<Grid>> {
    static std::ranges::range auto get(const Grid&, const GridFormat::CGAL::Cell<Grid>& cell) {
        static constexpr int num_corners = Concepts::CGALGrid2D<Grid> ? 3 : 4;
        return std::views::iota(0, num_corners) | std::views::transform([&] (int i) {
            return *cell.vertex(i);
        });
    }
};

template<Concepts::CGALGrid Grid>
struct PointCoordinates<Grid, typename Grid::Vertex> {
    static std::ranges::range auto get(const Grid&, const typename Grid::Vertex& vertex) {
        return CGAL::to_double_array(vertex.point());
    }
};

template<Concepts::CGALGrid Grid>
struct CellType<Grid, GridFormat::CGAL::Cell<Grid>> {
    static GridFormat::CellType get(const Grid&, const GridFormat::CGAL::Cell<Grid>&) {
        if constexpr (Concepts::CGALGrid2D<Grid>)
            return GridFormat::CellType::triangle;
        else
            return GridFormat::CellType::tetrahedron;
    }
};

template<Concepts::CGALGrid Grid>
struct NumberOfPoints<Grid> {
    static std::integral auto get(const Grid& grid) {
        return grid.number_of_vertices();
    }
};

template<Concepts::CGALGrid Grid>
struct NumberOfCells<Grid> {
    static std::integral auto get(const Grid& grid) {
        if constexpr (Concepts::CGALGrid2D<Grid>)
            return grid.number_of_faces();
        else
            return grid.number_of_finite_cells();
    }
};

template<Concepts::CGALGrid Grid>
struct NumberOfCellPoints<Grid, GridFormat::CGAL::Cell<Grid>> {
    static constexpr unsigned int get(const Grid&, const GridFormat::CGAL::Cell<Grid>&) {
        if constexpr (Concepts::CGALGrid2D<Grid>)
            return 3;
        else
            return 4;
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TRAITS_CGAL_HPP_

// SPDX-FileCopyrightText: 2025 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Wrapper around a grid that exposes only those cells fulfilling a given predicate.
 */
#ifndef GRIDFORMAT_GRID_FILTERED_HPP_
#define GRIDFORMAT_GRID_FILTERED_HPP_

#include <type_traits>
#include <concepts>
#include <ranges>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/ranges.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat {

/*!
 * \ingroup Grid
 * \brief Wrapper around a grid that exposes only those cells fulfilling a given predicate.
 * \note The wrapped grid fulfills the UnstructuredGrid concept and thus requires the given grid to do so as well.
 * \note All points of the original grid are exposed, i.e. they are not filtered to contain only those connected to
 *       the filtered cells.
 */
template<Concepts::UnstructuredGrid G, typename Predicate>
    requires(
        std::invocable<const std::remove_cvref_t<Predicate>&, const Cell<G>&> and
        std::convertible_to<bool, std::invoke_result_t<const std::remove_cvref_t<Predicate>&, const Cell<G>&>>
    )
class FilteredGrid {
    using StoredPredicate = LVReferenceOrValue<Predicate>;

 public:
    template<typename P> requires(std::convertible_to<Predicate, StoredPredicate>)
    explicit FilteredGrid(const G& grid, P&& p) noexcept
    : _grid{grid}
    , _p{std::forward<P>(p)} {
        _number_of_cells = Ranges::size(std::views::filter(Traits::Cells<G>::get(_grid), _p));
    }

    std::size_t number_of_cells() const {
        return _number_of_cells;
    }

    const G& unwrap() const {
        return _grid;
    }

    const std::remove_cvref_t<Predicate>& predicate() const {
        return _p;
    }

 private:
    const G& _grid;
    StoredPredicate _p;
    std::size_t _number_of_cells;
};

template<typename G, typename P> requires(Concepts::UnstructuredGrid<std::remove_cvref_t<G>>, std::is_lvalue_reference_v<G>)
FilteredGrid(G&&, P&&) -> FilteredGrid<std::remove_cvref_t<G>, P>;


#ifndef DOXYGEN
namespace Traits {

template<Concepts::UnstructuredGrid G, typename P>
struct Points<FilteredGrid<G, P>> {
    static std::ranges::range auto get(const FilteredGrid<G, P>& grid) {
        return Points<G>::get(grid.unwrap());
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct Cells<FilteredGrid<G, P>> {
    static std::ranges::range auto get(const FilteredGrid<G, P>& grid) {
        return std::views::filter(Cells<G>::get(grid.unwrap()), grid.predicate());
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct PointCoordinates<FilteredGrid<G, P>, Point<G>> {
    static decltype(auto) get(const FilteredGrid<G, P>& grid, const Point<G>& p) {
        return PointCoordinates<G, Point<G>>::get(grid.unwrap(), p);
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct CellPoints<FilteredGrid<G, P>, Cell<G>> {
    static auto get(const FilteredGrid<G, P>& grid, const Cell<G>& c) {
        return CellPoints<G, Cell<G>>::get(grid.unwrap(), c);
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct PointId<FilteredGrid<G, P>, Point<G>> {
    static auto get(const FilteredGrid<G, P>& grid, const Point<G>& p) {
        return PointId<G, Point<G>>::get(grid.unwrap(), p);
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct CellType<FilteredGrid<G, P>, Cell<G>> {
    static auto get(const FilteredGrid<G, P>& grid, const Cell<G>& c) {
        return CellType<G, Cell<G>>::get(grid.unwrap(), c);
    }
};

template<Concepts::UnstructuredGrid G, typename P>
struct NumberOfCells<FilteredGrid<G, P>> {
    static auto get(const FilteredGrid<G, P>& grid) {
        return grid.number_of_cells();
    }
};

}  // namespace Traits
#endif  // DOXYGEN

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_FILTERED_GRID_HPP_

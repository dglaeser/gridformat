// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \copybrief GridFormat::DiscontinousGrid
 */
#ifndef GRIDFORMAT_GRID_DISCONTINUOUS_HPP_
#define GRIDFORMAT_GRID_DISCONTINUOUS_HPP_

#include <utility>
#include <functional>
#include <type_traits>
#include <iterator>
#include <optional>
#include <cassert>

#include <gridformat/common/iterator_facades.hpp>
#include <gridformat/common/enumerated_range.hpp>
#include <gridformat/common/type_traits.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace DiscontinuousGridDetail {

    template<typename R> struct PointRangeStorage;
    template<typename R> requires(std::is_lvalue_reference_v<R>)
    struct PointRangeStorage<R> {
        using type = decltype(std::ref(std::declval<R>()));
    };
    template<typename R> requires(!std::is_lvalue_reference_v<R>)
    struct PointRangeStorage<R> {
        using type = std::remove_cvref_t<R>;
    };


    template<typename _C>
    class Cell {
        using HostCellStorage = LVReferenceOrValue<_C>;

     public:
        using HostCell = std::remove_cvref_t<_C>;

        template<typename C>
            requires(std::convertible_to<HostCellStorage, C>)
        Cell(C&& host_cell, std::size_t index)
        : _host_cell{std::forward<C>(host_cell)}
        , _index{index}
        {}

        std::size_t index() const { return _index; }
        const HostCell& host_cell() const { return *this; }
        operator const HostCell&() const { return _host_cell; }

     private:
        HostCellStorage _host_cell;
        std::size_t _index;
    };

    template<typename C>
    Cell(C&&, std::size_t) -> Cell<C>;

    template<typename T> struct IsDiscontinuousCell : public std::false_type {};
    template<typename C> struct IsDiscontinuousCell<Cell<C>> : public std::true_type {};


    template<typename _P, typename _C>
    class Point {
        using PStorage = LVReferenceOrValue<_P>;
        using CStorage = LVReferenceOrValue<_C>;
        using Cell = std::remove_cvref_t<_C>;
        using HostCell = typename Cell::HostCell;
        using HostPoint = std::remove_cvref_t<_P>;
        static_assert(IsDiscontinuousCell<Cell>::value);

     public:
        template<typename P, typename C>
            requires(std::convertible_to<PStorage, P> and
                     std::convertible_to<CStorage, C>)
        Point(P point, C cell, std::size_t i)
        : _point{std::forward<_P>(point)}
        , _cell{std::forward<_C>(cell)}
        , _index_in_host{i}
        {}

        const Cell& cell() const { return _cell; }
        const HostCell& host_cell() const { return _cell.host_cell(); }
        const HostPoint& host_point() const { return *this; }
        operator const HostPoint&() const { return _point; }
        std::size_t index_in_host() const { return _index_in_host; }

     private:
        LVReferenceOrValue<_P> _point;
        LVReferenceOrValue<_C> _cell;
        std::size_t _index_in_host;
    };

    template<typename P, typename C>
    Point(P&&, C&&, std::size_t) -> Point<P, C>;


    template<typename HostGrid>
    using CellType = Cell<
        std::ranges::range_reference_t<
            decltype(GridFormat::cells(std::declval<const HostGrid&>()))
        >
    >;

    template<typename HostGrid>
    using CellPointType = Point<
        std::ranges::range_reference_t<
            decltype(GridFormat::points(
                std::declval<const HostGrid&>(),
                std::declval<const GridFormat::Cell<HostGrid>&>()
            ))
        >,
        Cell<
            std::ranges::range_reference_t<
                decltype(GridFormat::cells(std::declval<const HostGrid&>()))
            >
        >
    >;


    template<typename HostGrid, typename CellIt, typename CellSentinel>
    class CellPointIterator
    : public ForwardIteratorFacade<CellPointIterator<HostGrid, CellIt, CellSentinel>,
                                   CellPointType<HostGrid>,
                                   CellPointType<HostGrid>> {
        using PointRange = decltype(GridFormat::points(
            std::declval<const HostGrid&>(),
            std::declval<const GridFormat::Cell<HostGrid>&>())
        );
        using PointRangeIterator = std::ranges::iterator_t<PointRange>;
        using PointRangeSentinel = std::ranges::sentinel_t<PointRange>;

     public:
        CellPointIterator() = default;
        CellPointIterator(const HostGrid& grid, CellIt it, CellSentinel sentinel)
        : _grid{&grid}
        , _cell_it{it}
        , _cell_end_it{sentinel} {
            if (!is_cell_end())
                _set_point_range();
        }

        // we have to implement the equality operator here to support ranges
        // where iterator_t and sentinel_t are not the same. This is why we
        // also have to give public access to the underlying iterators... :/
        template<typename IT, typename S>
        friend bool operator==(const CellPointIterator& self, const CellPointIterator<HostGrid, IT, S>& other) {
            if (self._grid_ptr() != other._grid_ptr()) return false;
            if (self.is_cell_end() != other.is_cell_end()) return false;
            if (self.is_cell_end()) return true;
            if (self._cell_iterator() != other._cell_iterator()) return false;
            return self._point_iterator().value() == other._point_iterator().value();
        }

        const auto _grid_ptr() const { return _grid; }
        const auto& _cell_iterator() const { return _cell_it; }
        const auto& _point_iterator() const { return _point_it; }
        bool is_cell_end() const { return _cell_it == _cell_end_it; }

     private:
        void _set_point_range() {
            _points = GridFormat::points(*_grid, (*_cell_it).host_cell());
            _point_it = std::ranges::begin(_points.value());
            _point_end_it = std::ranges::end(_points.value());
            _local_point_index = 0;
        }

        friend IteratorAccess;
        bool _is_equal(CellPointIterator& other) const {
            return *this == other;
        }

        void _increment() {
            if (is_cell_end())
                return;

            _local_point_index++;
            if (++_point_it.value(); _point_it.value() == _point_end_it.value()) {
                if (++_cell_it; _cell_it != _cell_end_it) {
                    _set_point_range();
                } else {
                    _points.reset();
                    _point_it.reset();
                    _point_end_it.reset();
                    _local_point_index = 0;
                }
            }
        }

        CellPointType<HostGrid> _dereference() const {
            return {*_point_it.value(), Cell{*_cell_it}, _local_point_index};
        }

        const HostGrid* _grid{nullptr};
        CellIt _cell_it;
        CellSentinel _cell_end_it;

        std::optional<typename PointRangeStorage<PointRange>::type> _points;
        std::optional<PointRangeIterator> _point_it;
        std::optional<PointRangeSentinel> _point_end_it;
        unsigned int _local_point_index{0};
    };

    template<typename Grid, typename IT, typename S>
    CellPointIterator(const Grid&, IT&&, S&&) -> CellPointIterator<Grid, std::remove_cvref_t<IT>, std::remove_cvref_t<S>>;


    template<typename Grid, typename CellRange>
    class CellPointRange {
     public:
        explicit CellPointRange(const Grid& grid, CellRange range)
        : _grid{&grid}
        , _range{std::forward<CellRange>(range)}
        {}

        auto begin() const {
            return CellPointIterator{*_grid, std::ranges::begin(_range), std::ranges::end(_range)};
        }

        auto end() const {
            return CellPointIterator{*_grid, std::ranges::end(_range), std::ranges::end(_range)};
        }

     private:
        const Grid* _grid{nullptr};
        LVReferenceOrValue<CellRange> _range;
    };

    template<typename G, typename R>
    CellPointRange(const G&, R&&) -> CellPointRange<G, R>;


    template<typename It>
    class CellIterator
    : public ForwardIteratorFacade<CellIterator<It>,
                                   Cell<typename std::iterator_traits<It>::reference>,
                                   Cell<typename std::iterator_traits<It>::reference>> {
     public:
        CellIterator() = default;
        CellIterator(It iterator) : _it{iterator} {}

        template<typename _IT>
        friend bool operator==(const CellIterator& self, const CellIterator<_IT>& other) {
            return self._get_it() == other._get_it();
        }

        const auto& _get_it() const {
            return _it;
        }

     private:
        friend IteratorAccess;

        void _increment() {
            ++_it;
            ++_index;
        }

        auto _dereference() const {
            return Cell{*_it, _index};
        }

        bool _is_equal(const CellIterator& other) const {
            return *this == other;
        }

        It _it;
        std::size_t _index{0};
    };

    template<typename It>
    CellIterator(It&&) -> CellIterator<It>;


    template<typename Grid>
    class CellRange {
        using _Range = decltype(GridFormat::cells(std::declval<const Grid&>()));

     public:
        explicit CellRange(const Grid& grid)
        : _range{GridFormat::cells(grid)}
        {}

        auto begin() const { return CellIterator{std::ranges::begin(_range)}; }
        auto end() const { return CellIterator{std::ranges::end(_range)}; }

     private:
        LVReferenceOrValue<_Range> _range;
    };

}  // namespace DiscontinuousGridDetail
#endif  // DOXYGEN

//! \addtogroup Grid
//! \{

template<Concepts::UnstructuredGrid Grid>
class DiscontinuousGrid {
    using This = DiscontinuousGrid<Grid>;
 public:
    using HostGrid = Grid;
    using Point = DiscontinuousGridDetail::CellPointType<HostGrid>;
    using Cell = DiscontinuousGridDetail::CellType<HostGrid>;

    explicit DiscontinuousGrid(const Grid& grid)
    : _grid{grid}
    {}

    std::ranges::forward_range auto points() const {
        return DiscontinuousGridDetail::CellPointRange{
            _grid,
            DiscontinuousGridDetail::CellRange{_grid}
        };
    }

    std::ranges::forward_range auto cells() const {
        return DiscontinuousGridDetail::CellRange{_grid};
    }

    operator const Grid&() const { return _grid; }
    const Grid& host_grid() const { return *this; }

 private:
    const Grid& _grid;
};

namespace Traits {

template<typename G>
struct Points<DiscontinuousGrid<G>> {
    static std::ranges::range auto get(const DiscontinuousGrid<G>& grid) {
        return grid.points();
    }
};

template<typename G>
struct Cells<DiscontinuousGrid<G>> {
    static std::ranges::range decltype(auto) get(const DiscontinuousGrid<G>& grid) {
        return grid.cells();
    }
};

template<typename G>
struct PointCoordinates<DiscontinuousGrid<G>, typename DiscontinuousGrid<G>::Point> {
    static decltype(auto) get(const DiscontinuousGrid<G>& grid,
                              const typename DiscontinuousGrid<G>::Point& p) {
        return PointCoordinates<G, Point<G>>::get(grid, p.host_point());
    }
};

template<typename G>
struct CellPoints<DiscontinuousGrid<G>, typename DiscontinuousGrid<G>::Cell> {
    static auto get(const DiscontinuousGrid<G>& grid,
                    const typename DiscontinuousGrid<G>::Cell& c) {
        return Ranges::enumerated(CellPoints<G, Cell<G>>::get(grid, c))
            | std::views::transform([&] <typename T> (T&& pair) {
                if constexpr (std::is_lvalue_reference_v<std::tuple_element<1, T>>)
                    return typename DiscontinuousGrid<G>::Point{
                        std::get<1>(pair), c, std::get<0>(pair)
                    };
                else
                    return typename DiscontinuousGrid<G>::Point{
                        std::move(std::get<1>(pair)), c, std::get<0>(pair)
                    };
            });
    }
};

template<typename G>
struct PointId<DiscontinuousGrid<G>, typename DiscontinuousGrid<G>::Point> {
    static std::size_t get(const DiscontinuousGrid<G>&, const typename DiscontinuousGrid<G>::Point& p) {
        // create a unique id by hashing the cell/point index tuple
        return std::hash<std::string>{}(std::to_string(p.cell().index()) + "/" + std::to_string(p.index_in_host()));
    }
};

template<typename G>
struct CellType<DiscontinuousGrid<G>, typename DiscontinuousGrid<G>::Cell> {
    static auto get(const DiscontinuousGrid<G>& grid,
                    const typename DiscontinuousGrid<G>::Cell& cell) {
        return CellType<G, Cell<G>>::get(grid, cell);
    }
};

}  // namespace Traits

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_DISCONTINUOUS_HPP_

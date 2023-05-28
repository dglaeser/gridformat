// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Adapters
 * \brief Traits specializations for [`dealii::Triangulation`](https://www.dealii.org/current/doxygen/deal.II/classTriangulation.html)
 */
#ifndef GRIDFORMAT_ADAPTERS_DEAL_II_HPP_
#define GRIDFORMAT_ADAPTERS_DEAL_II_HPP_

#include <cassert>
#include <ranges>
#include <vector>
#include <array>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/exceptions.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

// forward declaration of the triangulation class
// users of the adapter are expected to include the actual header
namespace dealii {

template<int dim, int spacedim>
class Triangulation;

}  // namespace dealii


namespace GridFormat {

namespace DealII {

// We need to wrap the deal.ii iterators because they do not fulfill the
// requirements for the std::ranges concepts. The issue comes from the
// std::indirectly_readable concept (https://en.cppreference.com/w/cpp/iterator/indirectly_readable)
// which expects that the reference type obtained from Iterator& and const Iterator&
// is the same: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1878r0.pdf.
// However, deal.ii iterators return refs or const refs depending on the constness of the iterator.
// For the traits, we only need read-access to cells/points, so we wrap the iterator and expose only
// the const interface in order to be compatible with std::ranges.
template<typename Iterator>
class ForwardIteratorWrapper
: public ForwardIteratorFacade<ForwardIteratorWrapper<Iterator>,
                               typename Iterator::value_type,
                               const typename Iterator::value_type&,
                               typename Iterator::pointer,
                               typename Iterator::difference_type> {
    using Reference = const typename Iterator::value_type&;

 public:
    ForwardIteratorWrapper() = default;
    ForwardIteratorWrapper(Iterator it) : _it{it} {}

    // we need to explicitly implement the equality operators because with
    // deal.ii we have compare iterator types that are not convertible into
    // each other (which is a requirement for the generic implementation provided
    // in GridFormat::IteratorFacade). For instance, active_cell_iterator has to
    // be compared against cell_iterator which is used as sentinel.
    template<typename I>
    friend bool operator==(const ForwardIteratorWrapper& self, const ForwardIteratorWrapper<I>& other) {
        return self._it == other._get_it();
    }

    template<typename I>
    friend bool operator!=(const ForwardIteratorWrapper& self, const ForwardIteratorWrapper<I>& other) {
        return self._it != other._get_it();
    }

    const Iterator& _get_it() const {
        return _it;
    }

 private:
    friend IteratorAccess;

    Reference _dereference() const { return *_it; }
    bool _is_equal(const ForwardIteratorWrapper& other) { return _it == other._it; }
    void _increment() { ++_it; }

    Iterator _it;
};

template<typename It>
ForwardIteratorWrapper(It&& it) -> ForwardIteratorWrapper<std::remove_cvref_t<It>>;

template<typename Triangulation>
using Cell = typename Triangulation::active_cell_iterator::AccessorType;

template<typename Triangulation>
using Point = typename Triangulation::active_vertex_iterator::AccessorType;

const std::vector<int>& cell_corners_in_gridformat_order(unsigned int cell_dimension,
                                                         unsigned int number_of_cell_corners) {
    if (cell_dimension == 0) {
        assert(number_of_cell_corners == 1);
        static const std::vector<int> corners{0};
        return corners;
    } else if (cell_dimension == 1) {
        assert(number_of_cell_corners == 2);
        static const std::vector<int> corners{0, 1};
        return corners;
    } else if (cell_dimension == 2) {
        // deal.ii currently only implements quads
        assert(number_of_cell_corners == 4);
        static const std::vector<int> corners{0, 1, 3, 2};
        return corners;
    } else if (cell_dimension == 3) {
        // deal.ii currently only implements hexes
        assert(number_of_cell_corners == 8);
        static const std::vector<int> corners{0, 1, 3, 2, 4, 5, 7, 6};
        return corners;
    }

    throw NotImplemented(
        "deal.ii cell corner indices for cell of dimension " + std::to_string(cell_dimension) +
        " and " + std::to_string(number_of_cell_corners) + " corners"
    );
    // In order to make compilers happy
    static constexpr std::vector<int> v{};
    return v;
}

}  // namespace DealII


// Specializations of the traits required for the `UnstructuredGrid` concept for dealii triangulations
namespace Traits {

template<int dim, int spacedim>
struct Points<dealii::Triangulation<dim, spacedim>> {
    static std::ranges::range decltype(auto) get(const dealii::Triangulation<dim, spacedim>& grid) {
        return std::ranges::subrange(
            DealII::ForwardIteratorWrapper{grid.begin_active_vertex()},
            DealII::ForwardIteratorWrapper{grid.end_vertex()}
        );
    }
};

template<int dim, int spacedim>
struct Cells<dealii::Triangulation<dim, spacedim>> {
    static std::ranges::range decltype(auto) get(const dealii::Triangulation<dim, spacedim>& grid) {
        return std::ranges::subrange(
            DealII::ForwardIteratorWrapper{grid.begin_active()},
            DealII::ForwardIteratorWrapper{grid.end()}
        );
    }
};

template<int dim, int spacedim>
struct CellType<dealii::Triangulation<dim, spacedim>, DealII::Cell<dealii::Triangulation<dim, spacedim>>> {
    static GridFormat::CellType get(const dealii::Triangulation<dim, spacedim>&,
                                    const DealII::Cell<dealii::Triangulation<dim, spacedim>>& cell) {
        using CT = GridFormat::CellType;
        static constexpr std::array cubes{CT::vertex, CT::segment, CT::quadrilateral, CT::hexahedron};
        static constexpr std::array simplices{CT::vertex, CT::segment, CT::triangle, CT::tetrahedron};
        const auto& ref_cell = cell.reference_cell();
        if (ref_cell.is_hyper_cube())
            return cubes[ref_cell.get_dimension()];
        else if (ref_cell.is_simplex())
            return simplices[ref_cell.get_dimension()];
        throw NotImplemented("CellType only implemented for hypercubes & simplices");
    }
};

template<int dim, int spacedim>
struct CellPoints<dealii::Triangulation<dim, spacedim>, DealII::Cell<dealii::Triangulation<dim, spacedim>>> {
    static std::ranges::range decltype(auto) get(const dealii::Triangulation<dim, spacedim>&,
                                                 const DealII::Cell<dealii::Triangulation<dim, spacedim>>& cell) {
        return DealII::cell_corners_in_gridformat_order(cell.reference_cell().get_dimension(), cell.n_vertices())
            | std::views::transform([&] (int i) { return *cell.vertex_iterator(i); });
    }
};

template<int dim, int spacedim>
struct PointId<dealii::Triangulation<dim, spacedim>, DealII::Point<dealii::Triangulation<dim, spacedim>>> {
    static auto get(const dealii::Triangulation<dim, spacedim>&,
                    const DealII::Point<dealii::Triangulation<dim, spacedim>>& point) {
        return point.index();
    }
};

template<int dim, int spacedim>
struct PointCoordinates<dealii::Triangulation<dim, spacedim>, DealII::Point<dealii::Triangulation<dim, spacedim>>> {
    static std::ranges::range decltype(auto) get(const dealii::Triangulation<dim, spacedim>&,
                                                 const DealII::Point<dealii::Triangulation<dim, spacedim>>& point) {
        static_assert(dim >= 1 && dim <= 3);
        const auto& center = point.center();
        if constexpr (dim == 1)
            return std::array{center[0]};
        else if constexpr (dim == 2)
            return std::array{center[0], center[1]};
        else if constexpr (dim == 3)
            return std::array{center[0], center[1], center[2]};
    }
};

template<int dim, int spacedim>
struct NumberOfPoints<dealii::Triangulation<dim, spacedim>> {
    static std::integral auto get(const dealii::Triangulation<dim, spacedim>& grid) {
        return grid.n_used_vertices();
    }
};

template<int dim, int spacedim>
struct NumberOfCells<dealii::Triangulation<dim, spacedim>> {
    static std::integral auto get(const dealii::Triangulation<dim, spacedim>& grid) {
        return grid.n_active_cells();
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_ADAPTERS_DEAL_II_HPP_
